#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "../include/config.h"

#define WORKDIR_BUFFERSIZE 1024
#define FILENAME_BUFFERSIZE 256

static char SERVER_PROGRAM_NAME[] = "server";

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

int main(int argc, char **argv) {
	char workdir[WORKDIR_BUFFERSIZE];
	{
		ssize_t len = readlink("/proc/self/exe", workdir, WORKDIR_BUFFERSIZE - 1);
		if (len == -1) {
			const char msg[] = "error: failed to read full program path\n";
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
	int errcode = readFilename(filename1, FILENAME_BUFFERSIZE);
	if (!errcode) {
		errcode = readFilename(filename2, FILENAME_BUFFERSIZE);
	}
	switch (errcode) {
	case 1: {
		const char msg[] = "error: failed to read filename from stdin\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 2: {
		const char msg[] = "error: no filename\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 3: {
		const char msg[] = "error: filename too long\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	}

	char filepath1[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
	snprintf(filepath1, WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE, "%s/%s", workdir, filename1);
	int32_t file1d = open(filepath1, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file1d == -1) {
		const char msg[] = "error: cannot open file1\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	    exit(EXIT_FAILURE);
	}

	char filepath2[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
	snprintf(filepath2, WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE, "%s/%s", workdir, filename2);
	int32_t file2d = open(filepath2, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file2d == -1) {
		const char msg[] = "error: cannot open file2\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	    exit(EXIT_FAILURE);
	}

	int pipeClientToServer1[2];
	if (pipe(pipeClientToServer1) == -1) {
		const char msg[] = "error: failed to create pipe pipeClientToServer1\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	int pipeClientToServer2[2];
	if (pipe(pipeClientToServer2) == -1) {
		const char msg[] = "error: failed to create pipe pipeClientToServer2\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	const pid_t server1_pid = fork();
	switch (server1_pid) {
	case -1: {
		const char msg[] = "error: failed to spawn new process server1\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 0: {
		{
			const pid_t pid = getpid();
			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg), "PID %d: I'm a server1\n", pid);
			write(STDOUT_FILENO, msg, length);
		}

		close(pipeClientToServer1[1]);
		dup2(pipeClientToServer1[0], STDIN_FILENO);
		close(pipeClientToServer1[0]);
		dup2(file1d, STDOUT_FILENO);
		close(file1d);

		close(pipeClientToServer2[0]);
		close(pipeClientToServer2[1]);
		close(file2d);

		char serverpath[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
		snprintf(serverpath, sizeof(serverpath) - 1, "%s/%s", workdir, SERVER_PROGRAM_NAME);
		char *const args[] = {SERVER_PROGRAM_NAME, NULL};
		int32_t status = execv(serverpath, args);
		if (status == -1) {
			const char msg[] = "error: failed to exec into new exectuable image\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
	}
	}

	const pid_t server2_pid = fork();
	switch (server2_pid) {
	case -1: {
		const char msg[] = "error: failed to spawn new process server2\n";
		write(STDERR_FILENO, msg, sizeof(msg));
	} exit(EXIT_FAILURE);
	case 0: {
		{
			const pid_t pid = getpid();
			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg), "PID %d: I'm a server2\n", pid);
			write(STDOUT_FILENO, msg, length);
		}

		close(pipeClientToServer2[1]);
		dup2(pipeClientToServer2[0], STDIN_FILENO);
		close(pipeClientToServer2[0]);
		dup2(file2d, STDOUT_FILENO);
		close(file2d);

		close(pipeClientToServer1[0]);
		close(pipeClientToServer1[1]);
		close(file1d);

		char serverpath[WORKDIR_BUFFERSIZE + FILENAME_BUFFERSIZE];
		snprintf(serverpath, sizeof(serverpath) - 1, "%s/%s", workdir, SERVER_PROGRAM_NAME);
		char *const args[] = {SERVER_PROGRAM_NAME, NULL};
		int32_t status = execv(serverpath, args);
		if (status == -1) {
			const char msg[] = "error: failed to exec into new exectuable image\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
	}
	}

	{
		const pid_t pid = getpid();
		char msg[128];
		const int32_t length = snprintf(msg, sizeof(msg),
			"PID %d: I'm a parent, my children has PID %d and %d\n", pid, server1_pid, server2_pid);
		write(STDOUT_FILENO, msg, length);
	}

	close(pipeClientToServer1[0]);
	close(file1d);
	close(pipeClientToServer2[0]);
	close(file2d);

	char buf[USER_INPUT_BUFFERSIZE];
	ssize_t bytes_read;
	bool oddLine = true;
	while (bytes_read = read(STDIN_FILENO, buf, USER_INPUT_BUFFERSIZE)) {
		if (bytes_read < 0) {
			const char msg[] = "error: failed to read from stdin (parent)\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		if (buf[0] == '\n') {
			write(pipeClientToServer1[1], buf, bytes_read);
			write(pipeClientToServer2[1], buf, bytes_read);
			break;
		}
		ssize_t bytes_written;
		if (oddLine) {
			bytes_written = write(pipeClientToServer1[1], buf, bytes_read);
		} else {
			bytes_written = write(pipeClientToServer2[1], buf, bytes_read);
		}
		if (bytes_written != bytes_read) {
			const char msg[] = "error: failed to write data to pipe\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		oddLine = !oddLine;
	}

	close(pipeClientToServer1[1]);
	close(pipeClientToServer2[1]);

	waitpid(server1_pid, NULL, 0);
	waitpid(server2_pid, NULL, 0);
	
	{
		const char msg[] = "Parent exit successfully\n";
		write(STDOUT_FILENO, msg, sizeof(msg));
	}

	return 0;
}