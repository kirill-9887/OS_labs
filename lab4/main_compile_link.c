#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "include/library.h"

#define BUFSIZE 1024

int readLine(char *buffer, const ssize_t size) {
    ssize_t bytes_read = read(STDIN_FILENO, buffer, size);
    if (bytes_read < 0) {
        return 1;
    }
    if (bytes_read == 0 || buffer[0] == '\n') {
        return 2;
    }
    if (bytes_read == size && buffer[size - 1] != '\n') {
        return 3;
    }
    if (buffer[bytes_read - 1] == '\n') {
        buffer[bytes_read - 1] = '\0';
    } else {
        buffer[bytes_read] = '\0';
    }
    return 0;
}

void print_invalid_input_msg() {
    const char msg[] = "error: invalid input\n";
    write(STDERR_FILENO, msg, sizeof(msg));
}

int main() {
    char user_input[BUFSIZE];
    int running = 1;
    
    do {
        int err = readLine(user_input, BUFSIZE);
        switch (err) {
            case 1: {
                const char msg[] = "error: failed to read stdin\n";
                write(STDERR_FILENO, msg, sizeof(msg));
            } exit(EXIT_FAILURE);
            case 2: {
                running = 0; 
            } break;
            case 3: {
                const char msg[] = "error: buffer overflow\n";
                write(STDERR_FILENO, msg, sizeof(msg));
            } exit(EXIT_FAILURE);
        }
        
        if (!running) {
            break;
        }

        char *arg = strtok(user_input, " \t\n");
        int command = atoi(arg);
        switch (command) {
            case 1:
            case 2: {
                int a = atoi(strtok(NULL, " \t\n"));
                int b = atoi(strtok(NULL, " \t\n"));
                int res;
                if (command == 1) {
                    res = prime_count(a, b);
                } else {
                    res = gcd(a, b);
                }
                char result_msg[BUFSIZE];
                int msg_size = snprintf(result_msg, BUFSIZE, "%d\n", res);
                write(STDOUT_FILENO, result_msg, msg_size);
            } break;
            default: {
                const char msg[] = "error: invalid input\n";
                write(STDERR_FILENO, msg, sizeof(msg));
            } exit(EXIT_FAILURE);
        }
    } while (running);

    return 0;
}