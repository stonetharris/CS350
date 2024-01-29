/*******************************************************************************
* Time Functions Library (implementation)
*
* Description:
*     A library to handle various time-related functions and operations.
*
* Author:
*     Renato Mancuso <rmancuso@bu.edu>
*
* Affiliation:
*     Boston University
*
* Creation Date:
*     September 10, 2023
*
* Notes:
*     Ensure to link against the necessary dependencies when compiling and
*     using this library. Modifications or improvements are welcome. Please
*     refer to the accompanying documentation for detailed usage instructions.
*
*******************************************************************************/

#include "timelib.h"

static inline uint64_t rdtsc(void)
{
    uint32_t low, high;
    __asm__ __volatile__ (
            "rdtsc" : "=a" (low), "=d" (high)
            );
    return ((uint64_t)high << 32) + low;
}

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using sleeping functions */
uint64_t get_elapsed_sleep(long sec, long nsec)
{
    struct timespec start, end;
    uint64_t start_cycles, end_cycles;

    /* Get start time and start cycles */
    clock_gettime(CLOCK_REALTIME, &start);
    start_cycles = rdtsc();

    /* Sleep for the desired time */
    struct timespec sleep_time = {.tv_sec = sec, .tv_nsec = nsec};
    nanosleep(&sleep_time, NULL);

    /* Get end time and end cycles */
    clock_gettime(CLOCK_REALTIME, &end);
    end_cycles = rdtsc();

    return end_cycles - start_cycles;
}

/* Return the number of clock cycles elapsed when waiting for
 * wait_time seconds using busy-waiting functions */
uint64_t get_elapsed_busywait(long sec, long nsec)
{
    struct timespec start, end, target_delay;
    uint64_t start_cycles, end_cycles;

    /* Get start time and start cycles */
    clock_gettime(CLOCK_REALTIME, &start);
    start_cycles = rdtsc();

    /* Busywait for the desired time */
    target_delay.tv_sec = start.tv_sec + sec;
    target_delay.tv_nsec = start.tv_nsec + nsec;
    if (target_delay.tv_nsec >= NANO_IN_SEC) {
        target_delay.tv_sec += 1;
        target_delay.tv_nsec -= NANO_IN_SEC;
    }
    while (timespec_cmp(&start, &target_delay) < 0) {
        clock_gettime(CLOCK_REALTIME, &start);
    }

    /* Get end time and end cycles */
    clock_gettime(CLOCK_REALTIME, &end);
    end_cycles = rdtsc();

    return end_cycles - start_cycles;
}

/* Utility function to add two timespec structures together. The input
 * parameter a is updated with the result of the sum. */
void timespec_add (struct timespec * a, struct timespec * b)
{
	/* Try to add up the nsec and see if we spill over into the
	 * seconds */
	time_t addl_seconds = b->tv_sec;
	a->tv_nsec += b->tv_nsec;
	if (a->tv_nsec > NANO_IN_SEC) {
		addl_seconds += a->tv_nsec / NANO_IN_SEC;
		a->tv_nsec = a->tv_nsec % NANO_IN_SEC;
	}
	a->tv_sec += addl_seconds;
}

/* Utility function to compare two timespec structures. It returns 1
 * if a is in the future compared to b; -1 if b is in the future
 * compared to a; 0 if they are identical. */
int timespec_cmp(struct timespec *a, struct timespec *b)
{
	if(a->tv_sec == b->tv_sec && a->tv_nsec == b->tv_nsec) {
		return 0;
	} else if((a->tv_sec > b->tv_sec) ||
		  (a->tv_sec == b->tv_sec && a->tv_nsec > b->tv_nsec)) {
		return 1;
	} else {
		return -1;
	}
}

/* Busywait for the amount of time described via the delay
 * parameter */
uint64_t busywait_timespec(struct timespec delay)
{
    struct timespec current_time, target_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    /* Calculate the target time */
    target_time = current_time;
    timespec_add(&target_time, &delay);

    while(timespec_cmp(&current_time, &target_time) < 0) {
        /* Keep checking the time until we've waited long enough */
        clock_gettime(CLOCK_REALTIME, &current_time);
    }
}
