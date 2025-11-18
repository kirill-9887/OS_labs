#ifndef TIME_CONV_H
#define TIME_CONV_H

#include <time.h>

double time2ms(
    struct timespec start_time,
    struct timespec end_time
);

#endif