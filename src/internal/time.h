#ifndef _INTERNAL_TIME_H
#define _INTERNAL_TIME_H

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

static inline uint64_t time_nsec()
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}

static inline uint64_t time_usec()
{
  return time_nsec() / 1000;
}

#endif
