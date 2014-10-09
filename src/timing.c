#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <stdint.h>
#include <sched.h>
#include "internal/parlib.h"
#include "export.h"
#include "arch.h"
#include "timing.h"

uint64_t get_tsc_freq(void)
{
	static uint64_t *tsc_freqs = NULL;
	if (tsc_freqs == NULL)
		tsc_freqs = calloc(get_nprocs(), sizeof(uint64_t));

	int cpuid = sched_getcpu();
	if (tsc_freqs[cpuid] == 0) {
		struct timeval prev;
		struct timeval curr;
		uint64_t beg = read_tsc_serialized();
		gettimeofday(&prev, 0);
		while (1) {
			gettimeofday(&curr, 0);
			if (curr.tv_sec > (prev.tv_sec + 1) ||
				(curr.tv_sec > prev.tv_sec && curr.tv_usec > prev.tv_usec))
				break;
		}
		uint64_t end = read_tsc_serialized();
		tsc_freqs[cpuid] = end - beg;
	}
	return tsc_freqs[cpuid];
}

void udelay(uint64_t usec)
{
	uint64_t start, end, now;

	start = read_tsc();
    end = start + (get_tsc_freq() * usec) / 1000000;
	do {
        cpu_relax();
        now = read_tsc();
	} while (now < end || (now > start && end < start));
}

/* Not super accurate, due to overheads of reading tsc and looping */
void ndelay(uint64_t nsec)
{
	uint64_t start, end, now;

	start = read_tsc();
    end = start + (get_tsc_freq() * nsec) / 1000000000;
	do {
        cpu_relax();
        now = read_tsc();
	} while (now < end || (now > start && end < start));
}

/* Difference between the ticks in microseconds */
uint64_t udiff(uint64_t begin, uint64_t end)
{
	return (end - begin) * 1000000 /  get_tsc_freq();
}

/* Difference between the ticks in nanoseconds */
uint64_t ndiff(uint64_t begin, uint64_t end)
{
	return (end - begin) * 1000000000 /  get_tsc_freq();
}

/* Conversion btw tsc ticks and time units.  From Akaros's kern/src/time.c */

/* We can overflow/wraparound when we multiply up, but we have to divide last,
 * or else we lose precision.  If we're too big and will overflow, we'll
 * sacrifice precision for correctness, and degrade to the next lower level
 * (losing 3 digits worth).  The recursive case shouldn't overflow, since it
 * called something that scaled down the tsc_time by more than 1000. */
uint64_t tsc2sec(uint64_t tsc_time)
{
	return tsc_time / get_tsc_freq();
}

uint64_t tsc2msec(uint64_t tsc_time)
{
	if (mult_will_overflow_u64(tsc_time, 1000))
		return tsc2sec(tsc_time) * 1000;
	else
		return (tsc_time * 1000) / get_tsc_freq();
}

uint64_t tsc2usec(uint64_t tsc_time)
{
	if (mult_will_overflow_u64(tsc_time, 1000000))
		return tsc2msec(tsc_time) * 1000;
	else
		return (tsc_time * 1000000) / get_tsc_freq();
}

uint64_t tsc2nsec(uint64_t tsc_time)
{
	if (mult_will_overflow_u64(tsc_time, 1000000000))
		return tsc2usec(tsc_time) * 1000;
	else
		return (tsc_time * 1000000000) / get_tsc_freq();
}

uint64_t sec2tsc(uint64_t sec)
{
	if (mult_will_overflow_u64(sec, get_tsc_freq()))
		return (uint64_t)(-1);
	else
		return sec * get_tsc_freq();
}

uint64_t msec2tsc(uint64_t msec)
{
	if (mult_will_overflow_u64(msec, get_tsc_freq()))
		return sec2tsc(msec / 1000);
	else
		return (msec * get_tsc_freq()) / 1000;
}

uint64_t usec2tsc(uint64_t usec)
{
	if (mult_will_overflow_u64(usec, get_tsc_freq()))
		return msec2tsc(usec / 1000);
	else
		return (usec * get_tsc_freq()) / 1000000;
}

uint64_t nsec2tsc(uint64_t nsec)
{
	if (mult_will_overflow_u64(nsec, get_tsc_freq()))
		return usec2tsc(nsec / 1000);
	else
		return (nsec * get_tsc_freq()) / 1000000000;
}


#undef get_tsc_freq 
#undef udelay
#undef ndelay
#undef udiff
#undef ndiff
#undef tsc2sec
#undef tsc2msec
#undef tsc2usec
#undef tsc2nsec
#undef sec2tsc
#undef msec2tsc
#undef usec2tsc
#undef nsec2tsc
EXPORT_ALIAS(INTERNAL(get_tsc_freq), get_tsc_freq)
EXPORT_ALIAS(INTERNAL(udelay), udelay)
EXPORT_ALIAS(INTERNAL(ndelay), ndelay)
EXPORT_ALIAS(INTERNAL(udiff), udiff)
EXPORT_ALIAS(INTERNAL(ndiff), ndiff)
EXPORT_ALIAS(INTERNAL(tsc2sec), tsc2sec)
EXPORT_ALIAS(INTERNAL(tsc2msec), tsc2msec)
EXPORT_ALIAS(INTERNAL(tsc2usec), tsc2usec)
EXPORT_ALIAS(INTERNAL(tsc2nsec), tsc2nsec)
EXPORT_ALIAS(INTERNAL(sec2tsc), sec2tsc)
EXPORT_ALIAS(INTERNAL(msec2tsc), msec2tsc)
EXPORT_ALIAS(INTERNAL(usec2tsc), usec2tsc)
EXPORT_ALIAS(INTERNAL(nsec2tsc), nsec2tsc)

