/*******************************************************************************
* CPU Clock Measurement Using RDTSC
*
* Description:
*     This C file provides functions to compute and measure the CPU clock using
*     the `rdtsc` instruction. The `rdtsc` instruction returns the Time Stamp
*     Counter, which can be used to measure CPU clock cycles.
*
* Author:
*     Renato Mancuso
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Notes:
*     Ensure that the platform supports the `rdtsc` instruction before using
*     these functions. Depending on the CPU architecture and power-saving
*     modes, the results might vary. Always refer to the CPU's official
*     documentation for accurate interpretations.
*
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "timelib.h"
#include <inttypes.h>

static inline uint64_t get_clocks2(void) {
    uint32_t __clocks_hi, __clocks_lo;
    __asm__ __volatile__ ("rdtsc" : "=a" (__clocks_lo), "=d" (__clocks_hi));
    return (((uint64_t)__clocks_hi) << 32) | ((uint64_t)__clocks_lo);
}


double get_elapsed_sleep2(int wait_time_seconds, int wait_time_nanos){
    struct timespec req, rem;
    req.tv_sec = wait_time_seconds;
    req.tv_nsec = wait_time_nanos;
    uint64_t start, end;
    start = get_clocks2();
    nanosleep(&req, &rem);
    end = get_clocks2();
    return end - start;
}


double get_elapsed_busywait2(int wait_time_seconds, int wait_time_nanos){
    struct timespec start_time, current_time, delay;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    delay.tv_sec = wait_time_seconds;
    delay.tv_nsec = wait_time_nanos;
    struct timespec end_time = start_time;
    timespec_add(&end_time, &delay);
    uint64_t start, end;
    start = get_clocks2();
    do {
        clock_gettime(CLOCK_MONOTONIC, &current_time);
    } while (timespec_cmp(&end_time, &current_time) > 0);
    end = get_clocks2();
    return end - start;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: %s <seconds> <nanoseconds> <s|b>\n", argv[0]);
        return 1;
    }

    int wait_time_seconds = atoi(argv[1]);
    int wait_time_nanos = atoi(argv[2]);
    char method = argv[3][0];

    uint64_t elapsed;
    double clock_speed;

    if (method == 's') {
        elapsed = get_elapsed_sleep2(wait_time_seconds, wait_time_nanos);
        printf("WaitMethod: SLEEP\n");
    } else if (method == 'b') {
        elapsed = get_elapsed_busywait2(wait_time_seconds, wait_time_nanos);
        printf("WaitMethod: BUSYWAIT\n");
    } else {
        printf("Invalid method\n");
        return 1;
    }

    double total_wait_time = wait_time_seconds + (wait_time_nanos / 1e9);
    clock_speed = (double)elapsed / total_wait_time / 1e6;

    printf("WaitTime: %d %d\n", wait_time_seconds, wait_time_nanos);
    printf("ClocksElapsed: %" PRIu64 "\n", elapsed);
    printf("ClockSpeed: %.2f\n", clock_speed);

    return 0;
}