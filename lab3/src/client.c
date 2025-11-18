#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include "../include/config.h"

#define WORKDIR_BUFFERSIZE 1024
#define FILENAME_BUFFERSIZE 256
#define MSG_BUFFERSIZE 256

static char SERVER_PROGRAM_NAME[] = "server";

typedef struct {
	char sem_client_to_server_1_name[NAME_BUFFERSIZE];
	sem_t *sem_client_to_server_1;
	char sem_client_to_server_2_name[NAME_BUFFERSIZE];
	sem_t *sem_client_to_server_2;
	char sem_server_to_client_name[NAME_BUFFERSIZE];
	sem_t *sem_server_to_client;
	char *shm_buf;
	char shm_name[NAME_BUFFERSIZE];
	int shm_fd;
	bool sem_client_to_server_1_opened;
	bool sem_client_to_server_2_opened;
	bool sem_server_to_client_opened;
	bool shm_buf_mapped;
	bool shm_opened;
} resources_t;

void resources_init(resources_t *resources) {
	resources->sem_client_to_server_1_opened = false;
	resources->sem_client_to_server_2_opened = false;
	resources->sem_server_to_client_opened = false;
	resources->shm_buf_mapped = false;
	resources->shm_opened = false;
}

resources_t RESOURCES;

int readFilename(char *filename, const ssize_t size) {
	ssize_t bytes_read = read(STDIN_FILENO, filename, size);
	if (bytes_read < 0) {
		return 1;
	}
	if (bytes_read == 0 || filename[0] == '\n') {
		return 2;
	}
	if (bytes_read == size && filename[size - 1] != '\n') {
		return 3;
	}
	if (filename[bytes_read - 1] == '\n') {
		filename[bytes_read - 1] = '\0';
	} else {
		filename[bytes_read] = '\0';
	}
	return 0;
}

void generate_name(char *res, size_t size, const char *pref, const char *suf) {
	int random_n = rand();
	long long time_n = (long long)time(NULL);
	snprintf(res, size, "%s%ld_%d%s", pref, time_n, random_n, suf);
}

int open_resources(char *msg, size_t msg_size, int *res_msg_size) {
	generate_name(RESOURCES.shm_name, NAME_BUFFERSIZE, "shm_", "");
	generate_name(RESOURCES.sem_client_to_server_1_name, NAME_BUFFERSIZE, "sem_client_to_server_1_", "");
	generate_name(RESOURCES.sem_client_to_server_2_name, NAME_BUFFERSIZE, "sem_client_to_server_2_", "");
	generate_name(RESOURCES.sem_server_to_client_name, NAME_BUFFERSIZE, "sem_server_to_client_", "");

	RESOURCES.shm_fd = shm_open(RESOURCES.shm_name, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (RESOURCES.shm_fd == -1) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to open shared memory\n");
		return -1;
	}
	RESOURCES.shm_opened = true;

	if (ftruncate(RESOURCES.shm_fd, sizeof(shared_data_t)) == -1) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to resize shared memory\n");
		return -1;
	}
	
	RESOURCES.shm_buf = mmap(NULL, sizeof(shared_data_t), PROT_WRITE, MAP_SHARED, RESOURCES.shm_fd, 0);
	if (RESOURCES.shm_buf == MAP_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to map shared memory\n");
		return -1;
	}
	RESOURCES.shm_buf_mapped = true;
	
	RESOURCES.sem_client_to_server_1 = sem_open(RESOURCES.sem_client_to_server_1_name, O_RDWR | O_CREAT, 0600, 0);
	if (RESOURCES.sem_client_to_server_1 == SEM_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to create semaphore client_to_server_1\n");
		return -1;
	}
	RESOURCES.sem_client_to_server_1_opened = true;
	
	RESOURCES.sem_client_to_server_2 = sem_open(RESOURCES.sem_client_to_server_2_name, O_RDWR | O_CREAT, 0600, 0);
	if (RESOURCES.sem_client_to_server_2 == SEM_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to create semaphore client_to_server_2\n");
		return -1;
	}
	RESOURCES.sem_client_to_server_2_opened = true;
	
	RESOURCES.sem_server_to_client = sem_open(RESOURCES.sem_server_to_client_name, O_RDWR | O_CREAT, 0600, 0);
	if (RESOURCES.sem_server_to_client == SEM_FAILED) {
		*res_msg_size = snprintf(msg, msg_size, "Error: Failed to create semaphore server_to_client\n");
		return -1;
	}
	RESOURCES.sem_server_to_client_opened = true;
	
	return 0;
}

void close_resources() {
	if (RESOURCES.sem_client_to_server_1_opened) {
		sem_close(RESOURCES.sem_client_to_server_1);
		RESOURCES.sem_client_to_server_1_opened = false;
	}
	sem_unlink(RESOURCES.sem_client_to_server_1_name);
	if (RESOURCES.sem_client_to_server_2_opened) {
		sem_close(RESOURCES.sem_client_to_server_2);
		RESOURCES.sem_client_to_server_2_opened = false;
	}
	sem_unlink(RESOURCES.sem_client_to_server_2_name);
	if (RESOURCES.sem_server_to_client_opened) {
		sem_close(RESOURCES.sem_server_to_client);
		RESOURCES.sem_server_to_client_opened = false;
	}
	sem_unlink(RESOURCES.sem_server_to_client_name);
	if (RESOURCES.shm_buf_mapped) {
		munmap(RESOURCES.shm_buf, sizeof(shared_data_t));
		RESOURCES.shm_buf_mapped = false;
	}
	if (RESOURCES.shm_opened) {
		close(RESOURCES.shm_fd);
		RESOURCES.shm_opened = false;
	}
	shm_unlink(RESOURCES.shm_name);
}

void safe_exit(int sig) {
	exit(0);
}

int main(int argc, char **argv) {
	srand(time(NULL));
	char workdir[WORKDIR_BUFFERSIZE];
	{
		ssize_t len = readlink("/proc/self/exe", workdir, WORKDIR_BUFFERSIZE - 1);
		if (len == -1) {
			const char msg[] = "Error: Failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		while (workdir[len] != '/') {
			--len;
		}
		workdir[len] = '\0';
	}

	char filename1[FILENAME_BUFFERSIZE];
	char filename2[FILENAME_BUFFERSIZE];
	{
		const char msg[] = "Enter first filename > ";
		write(STDOUT_FILENO, msg, sizeof(msg));
	}
	int errcode = readFilename(filename1, FILENAME_BUFFERSIZE);
	if (!errcode) {
		{
			const char msg[] = "Enter second filename > ";
			write(STDOUT_FILENO, msg, sizeof(msg));
		}
		errcode = readFilename(filename2, FILENAME_BUFFERSIZE);
	}
	switch (errcode) {
	case 1: {
		const char msg[] = "Error: Failed to read filename from stdin\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 2: {
		const char msg[] = "Error: No filename\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 3: {
		const char msg[] = "Error: Filename too long\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	}

	char filepath1[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
	snprintf(filepath1, WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE, "%s/%s", workdir, filename1);
	int32_t file1d = open(filepath1, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file1d == -1) {
		const char msg[] = "Error: Cannot open file_1\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	    exit(EXIT_FAILURE);
	}

	char filepath2[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
	snprintf(filepath2, WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE, "%s/%s", workdir, filename2);
	int32_t file2d = open(filepath2, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file2d == -1) {
		const char msg[] = "Error: Cannot open file_2\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	    exit(EXIT_FAILURE);
	}

	atexit(close_resources);
	signal(SIGINT, safe_exit);

	char msg[MSG_BUFFERSIZE];
	int msg_size;
	resources_init(&RESOURCES);
	if (open_resources(msg, MSG_BUFFERSIZE, &msg_size)) {
		write(STDERR_FILENO, msg, msg_size);
		exit(EXIT_FAILURE);
	}

	const pid_t server1_pid = fork();
	switch (server1_pid) {
	case -1: {
		const char msg[] = "Error: Failed to spawn new process server_1\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 0: {
		{
			const pid_t pid = getpid();
			char msg[64];
			const int length = snprintf(msg, sizeof(msg), "PID %d: I'm a server_1\n", pid);
			write(STDOUT_FILENO, msg, length);
		}
		dup2(file1d, STDOUT_FILENO);
		close(file1d);
		close(file2d);

		char serverpath[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
		snprintf(serverpath, sizeof(serverpath), "%s/%s", workdir, SERVER_PROGRAM_NAME);
		char *const args[] = {
			SERVER_PROGRAM_NAME,
			RESOURCES.shm_name,
			RESOURCES.sem_client_to_server_1_name,
			RESOURCES.sem_server_to_client_name,
			NULL
		};
		int32_t status = execv(serverpath, args);
		if (status == -1) {
			const char msg[] = "Error: Failed to exec into new exectuable image\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
	}
	}

	const pid_t server2_pid = fork();
	switch (server2_pid) {
	case -1: {
		const char msg[] = "Error: Failed to spawn new process server_2\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 0: {
		{
			const pid_t pid = getpid();
			char msg[64];
			const int length = snprintf(msg, sizeof(msg), "PID %d: I'm a server_2\n", pid);
			write(STDOUT_FILENO, msg, length);
		}
		dup2(file2d, STDOUT_FILENO);
		close(file2d);
		close(file1d);

		char serverpath[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
		snprintf(serverpath, sizeof(serverpath), "%s/%s", workdir, SERVER_PROGRAM_NAME);
		char *const args[] = {
			SERVER_PROGRAM_NAME,
			RESOURCES.shm_name,
			RESOURCES.sem_client_to_server_2_name,
			RESOURCES.sem_server_to_client_name,
			NULL
		};
		int32_t status = execv(serverpath, args);
		if (status == -1) {
			const char msg[] = "Error: Failed to exec into new exectuable image\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
	}
	}
	{
		const pid_t pid = getpid();
		char msg[128];
		const int length = snprintf(msg, sizeof(msg),
			"PID %d: I'm a parent, my children has PID %d and %d\n", pid, server1_pid, server2_pid);
		write(STDOUT_FILENO, msg, length);
	}
	close(file1d);
	close(file2d);

	shared_data_t *data = (shared_data_t *)RESOURCES.shm_buf;
	char buf[USER_INPUT_BUFFERSIZE];
	ssize_t bytes_read;
	bool oddLine = true;
	while (bytes_read = read(STDIN_FILENO, buf, USER_INPUT_BUFFERSIZE)) {
		bool finish_input = buf[0] == '\n';
		bool failed_read = bytes_read < 0;
		bool overflow_input = bytes_read == USER_INPUT_BUFFERSIZE && buf[bytes_read - 1] != '\n';
		if (finish_input || failed_read || overflow_input) {
			data->length = 0;
			sem_post(RESOURCES.sem_client_to_server_1);
			sem_post(RESOURCES.sem_client_to_server_2);
			if (failed_read || overflow_input) {
				char msg[MSG_BUFFERSIZE];
				int msg_size;
				if (failed_read) {
					msg_size = snprintf(msg, MSG_BUFFERSIZE, "Error: Failed to read from stdin (parent)\n");
				} else if (overflow_input) {
					msg_size = snprintf(msg, MSG_BUFFERSIZE, "Error: Received line too long\n");
				}
				write(STDERR_FILENO, msg, msg_size);
				exit(EXIT_FAILURE);
			}
			break;
		}
		data->length = bytes_read;
		memcpy(data->text, buf, bytes_read);
		if (oddLine) {
			sem_post(RESOURCES.sem_client_to_server_1);
		} else {
			sem_post(RESOURCES.sem_client_to_server_2);
		}
		sem_wait(RESOURCES.sem_server_to_client);
		oddLine = !oddLine;
	}
	waitpid(server1_pid, NULL, 0);
	waitpid(server2_pid, NULL, 0);
	{
		const char msg[] = "Parent exit successfully\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
	}
	return 0;
}