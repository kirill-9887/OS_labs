#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#include "../include/config.h"

#define MSG_BUFFERSIZE 256

enum process_data_error {
	NO_DATA_ERROR = 1,
	WRITE_ERROR,
};

typedef struct {
	char sem_client_to_server_name[NAME_BUFFERSIZE];
	sem_t *sem_client_to_server;
	char sem_server_to_client_name[NAME_BUFFERSIZE];
	sem_t *sem_server_to_client;
	char *shm_buf;
	char shm_name[NAME_BUFFERSIZE];
	int shm_fd;
	bool shm_opened;
	bool shm_buf_mapped;
	bool sem_client_to_server_opened;
	bool sem_server_to_client_opened;
} resources_t;

resources_t RESOURCES;

void resources_init(resources_t *resources) {
	resources->shm_opened = false;
	resources->shm_buf_mapped = false;
	resources->sem_client_to_server_opened = false;
	resources->sem_server_to_client_opened = false;
}

int open_resources(char *msg, size_t msg_size, int *res_msg_size) {
	RESOURCES.shm_fd = shm_open(RESOURCES.shm_name, O_RDWR, 0);
	if (RESOURCES.shm_fd == -1) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to open SHM\n");
		return -1;
	}
	RESOURCES.shm_opened = true;

	RESOURCES.shm_buf = mmap(NULL, sizeof(shared_data_t), 
		PROT_READ | PROT_WRITE, MAP_SHARED, RESOURCES.shm_fd, 0);
	if (RESOURCES.shm_buf == MAP_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to map SHM\n");
		return -1;
	}
	RESOURCES.shm_buf_mapped = true;

	RESOURCES.sem_client_to_server = sem_open(RESOURCES.sem_client_to_server_name, O_RDWR);
	if (RESOURCES.sem_client_to_server == SEM_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to open semaphore\n");
		return -1;
	}
	RESOURCES.sem_client_to_server_opened = true;

	RESOURCES.sem_server_to_client = sem_open(RESOURCES.sem_server_to_client_name, O_RDWR);
	if (RESOURCES.sem_server_to_client == SEM_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to open semaphore\n");
		return -1;
	}
	RESOURCES.sem_server_to_client_opened = true;

	return 0;
}

void close_resources() {
	if (RESOURCES.sem_server_to_client_opened) {
		sem_close(RESOURCES.sem_server_to_client);
		RESOURCES.sem_server_to_client_opened = false;
	}
	if (RESOURCES.sem_client_to_server_opened) {
		sem_close(RESOURCES.sem_client_to_server);
		RESOURCES.sem_client_to_server_opened = false;
	}
	if (RESOURCES.shm_buf_mapped) {
		munmap(RESOURCES.shm_buf, sizeof(shared_data_t));
		RESOURCES.shm_buf_mapped = false;
	}
	if (RESOURCES.shm_opened) {
		close(RESOURCES.shm_fd);
		RESOURCES.shm_opened = false;
	}
}

void safe_exit(int sig) {
	exit(0);
}

int process_data(shared_data_t *data) {
	if (data->length == 0) {
		return NO_DATA_ERROR;
	}
	ssize_t j = data->text[data->length - 1] == '\n' ? data->length - 2 : data->length - 1;
	for (ssize_t i = 0; i < j; ++i, --j) {
		char tmp = data->text[i];
		data->text[i] = data->text[j];
		data->text[j] = tmp;
	}
	ssize_t bytes_written = write(STDOUT_FILENO, data->text, data->length);
	if (bytes_written != data->length) {
		return WRITE_ERROR;
	}
	return 0;
}

int main(int argc, char **argv) {
	const pid_t pid = getpid();

	snprintf(RESOURCES.shm_name, NAME_BUFFERSIZE, "%s", argv[1]);
	snprintf(RESOURCES.sem_client_to_server_name, NAME_BUFFERSIZE, "%s", argv[2]);
	snprintf(RESOURCES.sem_server_to_client_name, NAME_BUFFERSIZE, "%s", argv[3]);

	atexit(close_resources);
	signal(SIGINT, safe_exit);
	
	char msg[MSG_BUFFERSIZE];
	int msg_size;
	resources_init(&RESOURCES);
	if (open_resources(msg, MSG_BUFFERSIZE, &msg_size)) {
		write(STDERR_FILENO, msg, MSG_BUFFERSIZE);
		exit(EXIT_FAILURE);
	}

	shared_data_t *data = (shared_data_t *)RESOURCES.shm_buf;
	int error = 0;
	bool running = true;
	while (running) {
		sem_wait(RESOURCES.sem_client_to_server);
		error = process_data(data);
		if (error) {
			running = false;
		}
		sem_post(RESOURCES.sem_server_to_client);
	}
	if (error == WRITE_ERROR) {
		char msg[MSG_BUFFERSIZE];
		const int msg_size = snprintf(msg, MSG_BUFFERSIZE, 
			"Error: Failed to write to stdout (PID: %d)\n", pid);
		write(STDERR_FILENO, msg, msg_size);
		exit(EXIT_FAILURE);
	}
	{
		char msg[MSG_BUFFERSIZE];
		const int msg_size = snprintf(msg, MSG_BUFFERSIZE, 
			"Child PID %d exit successfully\n", pid);
		write(STDERR_FILENO, msg, msg_size);
	}
	return 0;
}