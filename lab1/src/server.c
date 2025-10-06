#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include "../include/config.h"

#define MSG_BUFFERSIZE 256

int main(int argc, char **argv) {
	const pid_t pid = getpid();
	char buf[USER_INPUT_BUFFERSIZE];
	ssize_t bytes_read;	
	while (bytes_read = read(STDIN_FILENO, buf, sizeof(buf))) {
		if (bytes_read < 0) {
			char msg[MSG_BUFFERSIZE];
			const int32_t msg_size = snprintf(msg, MSG_BUFFERSIZE, "error: failed to read from stdin (PID: %d)\n", pid);
			write(STDERR_FILENO, msg, msg_size);
			exit(EXIT_FAILURE);
		}
		if (buf[0] == '\n') {
			break;
		}
		for (ssize_t i = 0, j = bytes_read - 2; i < j; ++i, --j) {
			char tmp = buf[i];
			buf[i] = buf[j];
			buf[j] = tmp;
		}
		ssize_t bytes_written = write(STDOUT_FILENO, buf, bytes_read);
		if (bytes_written != bytes_read) {
			char msg[MSG_BUFFERSIZE];
			const int32_t msg_size = snprintf(msg, MSG_BUFFERSIZE, "error: failed to write to stdout (PID: %d)\n", pid);
			write(STDERR_FILENO, msg, msg_size);
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}