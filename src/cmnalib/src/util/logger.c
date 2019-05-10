
#include <stdio.h>
#include <stdarg.h>

#include <sys/time.h>
#include <sys/types.h>

#include "cmnalib/logger.h" 

// global flag
int enable_logger = 1;

void do_log(FILE* stream, const char * format, ...) {
    if(enable_logger) {
#if DO_LOG_ENABLE_TIMESTAMP
        struct timeval t;
        gettimeofday(&t, NULL);
        fprintf(stream, "%ld.%06ld ", t.tv_sec, t.tv_usec);
#endif
        va_list myargs;
        va_start(myargs, format);
        vfprintf(stream, format, myargs);
        va_end(myargs);
    }
}


int timeval_subtract (struct timeval *result,
                      struct timeval *x,
                      struct timeval *y) {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}
