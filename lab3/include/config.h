#ifndef CONFIG_H
#define CONFIG_H

#define USER_INPUT_BUFFERSIZE 4096 - sizeof(ssize_t)
#define NAME_BUFFERSIZE 128

typedef struct {
    ssize_t length;
    char text[USER_INPUT_BUFFERSIZE];
} shared_data_t;

#endif