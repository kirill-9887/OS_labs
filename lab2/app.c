#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "./include/time_conv.h"
#include "./include/card_model.h"

#define TEST_COUNT 3
#define MSG_BUFFERSIZE 256

static const char alloc_mem_err_msg[] = "Error: Allocation memory error.\n";
static const char create_thread_err_msg[] = "Error: Create thread error.\n";

typedef struct {
    int thread_number;
    int rounds_count;
    int satisfactory_case_count;
    unsigned int seed;
    char padding[64 - 3 * sizeof(int) - sizeof(unsigned int)];
} ThreadArgs;

int min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

static void *thread_func_do_round(void *_args) {
    ThreadArgs *args = (ThreadArgs*)_args;
    {
        char msg[MSG_BUFFERSIZE];
        size_t msg_size = snprintf(msg, MSG_BUFFERSIZE, "Thread %d started.\n", args->thread_number);
        write(STDOUT_FILENO, msg, msg_size);
    }
    int satisfactory_case_count = 0;
    for (int completed_rounds = 0; completed_rounds < args->rounds_count; ++completed_rounds) {
        bool cards_sames = random_cards_deck_round(&(args->seed));
        satisfactory_case_count += (int)cards_sames;
    }
    args->satisfactory_case_count = satisfactory_case_count;
    return NULL;
}

int parallel_calculation(
    int max_threads_count,
    int rounds_count,
    int *res_threads_count,
    double *res_probability,
    double *res_threads_time_ms
) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    int threads_count = min(rounds_count, max_threads_count);
    pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);
    ThreadArgs *threads_args = (ThreadArgs*)malloc(sizeof(ThreadArgs) * threads_count);
    if (threads == NULL || threads_args == NULL) {
        write(STDERR_FILENO, alloc_mem_err_msg, sizeof(alloc_mem_err_msg) - 1);
        return -1;
    }
    int min_rounds_count_per_thread = rounds_count / threads_count;
    int extra_rounds_count = rounds_count % threads_count;
    
    int thread_created = 0;
    for (int i = 0; i < threads_count; ++i) {
        threads_args[i].thread_number = i;
        threads_args[i].rounds_count = min_rounds_count_per_thread + (int)(i < extra_rounds_count);
        threads_args[i].satisfactory_case_count = 0;
        threads_args[i].seed = (unsigned int)time(NULL) + i * 1000;
        if (pthread_create(&threads[i], NULL, thread_func_do_round, (void *)&(threads_args[i]))) {
            break;
        }
        ++thread_created;
    }
    for (int i = 0; i < thread_created; ++i) {
        pthread_join(threads[i], NULL);
    }
    if (thread_created < threads_count) {
        write(STDERR_FILENO, create_thread_err_msg, sizeof(create_thread_err_msg) - 1);
        return -1;
    }

    int satisfactory_case_count = 0;
    for (int i = 0; i < threads_count; ++i) {
        satisfactory_case_count += threads_args[i].satisfactory_case_count;
    }
    double probability = satisfactory_case_count / (double)rounds_count;

    free(threads);
    free(threads_args);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double threads_time_ms = time2ms(start_time, end_time);
    
    *res_threads_count = threads_count;
    *res_probability = probability;
    *res_threads_time_ms = threads_time_ms;

    return 0;
}

int main(int argc, const char **argv) {
    if (argc - 1 != 2) {
        const char msg[] = "Error: Wrong arguments count. Should be 2: max_threads_count and rounds_count.\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return -1;
    }
    size_t n_processors = sysconf(_SC_NPROCESSORS_ONLN);
    size_t ThreadArgs_size = sizeof(ThreadArgs);
    {
        char msg[MSG_BUFFERSIZE];
        size_t msg_size = snprintf(msg, MSG_BUFFERSIZE, 
            "N_processors: %zu.\n", n_processors);
        write(STDOUT_FILENO, msg, msg_size);
        msg_size = snprintf(msg, MSG_BUFFERSIZE, 
            "ThreadArgs size: %zu.\n", ThreadArgs_size);
        write(STDOUT_FILENO, msg, msg_size);
    }
    int max_threads_count = atoi(argv[1]);
    int rounds_count = atoi(argv[2]);

    int threads_count = -1;
    double mean_probability = 0;
    double mean_threads_time_ms = 0;
    for (int i = 0; i < TEST_COUNT; ++i) {
        double probability;
        double threads_time_ms;
        int error = parallel_calculation(
            max_threads_count, 
            rounds_count, 
            &threads_count, 
            &probability, 
            &threads_time_ms
        );
        if (error) {
            return error;
        }
        mean_probability += probability / TEST_COUNT;
        mean_threads_time_ms += threads_time_ms / TEST_COUNT;
    }
    
    {
        int32_t msg_size;
        char msg[MSG_BUFFERSIZE];
        msg_size = snprintf(msg, MSG_BUFFERSIZE, 
            "Threads count: %d.\n", threads_count);
        write(STDOUT_FILENO, msg, msg_size);
        msg_size = snprintf(msg, MSG_BUFFERSIZE, 
            "Mean probability: %f.\n", mean_probability);
        write(STDOUT_FILENO, msg, msg_size);
        msg_size = snprintf(msg, MSG_BUFFERSIZE, 
            "Mean threads execution time: %.3f мс.\n", mean_threads_time_ms);
        write(STDOUT_FILENO, msg, msg_size);
    }
    return 0;
}