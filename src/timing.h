#ifndef PARLIB_TIMING_H
#define PARLIB_TIMING_H
#include <stdbool.h>
#include <stdint.h>

#ifdef COMPILING_PARLIB
#define get_tsc_freq INTERNAL(get_tsc_freq)
#define udelay INTERNAL(udelay)
#define ndelay INTERNAL(ndelay)
#define udiff INTERNAL(udiff)
#define ndiff INTERNAL(ndiff)
#define tsc2sec INTERNAL(tsc2sec)
#define tsc2msec INTERNAL(tsc2msec)
#define tsc2usec INTERNAL(tsc2usec)
#define tsc2nsec INTERNAL(tsc2nsec)
#define sec2tsc INTERNAL(sec2tsc)
#define msec2tsc INTERNAL(msec2tsc)
#define usec2tsc INTERNAL(usec2tsc)
#define nsec2tsc INTERNAL(nsec2tsc)
#endif

uint64_t get_tsc_freq();
void udelay(uint64_t usec);
void ndelay(uint64_t nsec);
uint64_t udiff(uint64_t begin, uint64_t end);
uint64_t ndiff(uint64_t begin, uint64_t end);

/* Conversion btw tsc ticks and time units.  From Akaros's kern/src/time.c */
uint64_t tsc2sec(uint64_t tsc_time);
uint64_t tsc2msec(uint64_t tsc_time);
uint64_t tsc2usec(uint64_t tsc_time);
uint64_t tsc2nsec(uint64_t tsc_time);
uint64_t sec2tsc(uint64_t sec);
uint64_t msec2tsc(uint64_t msec);
uint64_t usec2tsc(uint64_t usec);
uint64_t nsec2tsc(uint64_t nsec);

# ifdef __i386__

static inline uint64_t read_tsc(void)
{
	uint64_t tsc;
	asm volatile("rdtsc" : "=A" (tsc));
	return tsc;
}

static inline uint64_t read_tsc_serialized(void)
{
	uint64_t tsc;
	asm volatile("lfence; rdtsc" : "=A" (tsc));
	return tsc;
}

# elif __x86_64__

static inline uint64_t read_tsc(void)
{
	uint32_t lo, hi;
	/* We cannot use "=A", since this would use %rax on x86_64 */
	asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
	return (uint64_t)hi << 32 | lo;
}

static inline uint64_t read_tsc_serialized(void)
{
	uint32_t lo, hi;
	asm volatile("lfence; rdtsc" : "=a" (lo), "=d" (hi));
	return (uint64_t)hi << 32 | lo;
}

# else
#  error "Which arch is this?"
# endif /* __i386__ | __x86_64__ */

static inline int mult_will_overflow_u64(uint64_t a, uint64_t b)
{
	if (!a)
		return false;
	return (uint64_t)(-1) / a < b;
}

#endif /* PARLIB_TIMING_H */
