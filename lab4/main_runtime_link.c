#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h> // read, write
#include <dlfcn.h> // dlopen, dlsym, dlclose, RTLD_*

#define BUFSIZE 1024
const char *LIB_NAMES[2] = { "./liblibrary_1.so", "./liblibrary_2.so" };

typedef int (*prime_count_func)(int, int);
typedef int (*gcd_func)(int, int);

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

static prime_count_func prime_count;
static gcd_func gcd;

static int func_impl_stub(int a, int b) {
	(void)a;
	(void)b;
	return 0;
}

int connect_library(const char *lib_name, void **library) {
    if (!lib_name || !library) {
        return -1;
    }
    if (*library) {
        dlclose(*library);
    }
    *library = NULL;
    *library = dlopen(lib_name, RTLD_LOCAL | RTLD_NOW);
    if (!*library) {
        return -1;
    }
    prime_count = dlsym(*library, "prime_count");
    if (prime_count == NULL) {
        const char msg[] = "warning: failed to find prime_count function implementation\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        prime_count = func_impl_stub;
    }
    gcd = dlsym(*library, "gcd");
    if (gcd == NULL) {
        const char msg[] = "warning: failed to find gcd function implementation\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        gcd = func_impl_stub;
    }
    return 0;
}

int connect_next_lib(void **library) {
    static int lib_number = 1;
    lib_number = lib_number == 0 ? 1 : 0;
    return connect_library(LIB_NAMES[lib_number], library);
}

int main() {
    void *library = NULL;
	if (connect_next_lib(&library)) {
        const char msg[] = "error: library failed to load\n";
		write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

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
            case 0: {
                if (connect_next_lib(&library)) {
                    const char msg[] = "error: library failed to load\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            } break;
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

	if (library) {
		dlclose(library);
	}

    return 0;
}