#include "../include/time_conv.h"

double time2ms(
    struct timespec start_time,
    struct timespec end_time
) {
    long long total_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000LL + 
                         (end_time.tv_nsec - start_time.tv_nsec);
    long threads_time_ms = total_ns / 1000000;
    long threads_time_mks = (total_ns % 1000000) / 1000;
    return threads_time_ms + threads_time_mks / (double)1000;
}