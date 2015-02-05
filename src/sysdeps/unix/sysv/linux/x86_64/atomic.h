/*
 * Copyright (c) 2011 The Regents of the University of California
 * Benjamin Hindman <benh@cs.berkeley.edu>
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 *
 * This file is part of Parlib.
 * 
 * Parlib is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Parlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * See COPYING.LESSER for details on the GNU Lesser General Public License.
 * See COPYING for details on the GNU General Public License.
 */

/**
 * Memory consistency issues will vary between machines ... here are
 * a few helpful guarantees for x86 machines:
 *
 *  - Loads are _not_ reordered with other loads.
 *  - Stores are _not_ reordered with other stores.
 *  - Stores are _not_ reordered with older loads.
 *  - In a multiprocessor system, memory ordering obeys causality (memory
 *    ordering respects transitive visibility).
 *  - In a multiprocessor system, stores to the same location have a
 *    total order.
 *  - In a multiprocessor system, locked instructions have a total order.
 *  - Loads and stores are not reordered with locked instructions.
 *
 * However, loads _may be_ reordered with older stores to different
 * locations. This sometimes necesitates the need for memory fences
 * (and no the keyword 'volatile' will not _always_ do that for
 * you!). In addition, a memory fence may sometimes be helpful to
 * avoid letting the compiler do any of its own fancy reorderings.
*/

#ifndef ATOMIC_H
#define ATOMIC_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Full CPU memory barrier */
#define mb() ({ asm volatile("mfence" ::: "memory"); })
/* Compiler memory barrier (optimization barrier) */
#define cmb() ({ asm volatile("" ::: "memory"); })
/* Partial CPU memory barriers */
#define rmb() cmb()
#define wmb() cmb()
#define wrmb() mb()
#define rwmb() cmb()

/* Forced barriers, used for string ops, SSE ops, dealing with hardware, or
 * other places where you avoid 'normal' x86 read/writes (like having an IPI
 * beat a write) */
#define mb_f() ({ asm volatile("mfence" ::: "memory"); })
#define rmb_f() ({ asm volatile("lfence" ::: "memory"); })
#define wmb_f() ({ asm volatile("sfence" ::: "memory"); })
#define wrmb_f() mb_f()
#define rwmb_f() mb_f()

typedef void* atomic_t;
#define ATOMIC_INITIALIZER(val) ((atomic_t)(val))

static inline void atomic_init(atomic_t *number, long val)
{
    *(volatile long*)number = val;
}

static inline void *atomic_swap_ptr(void **addr, void *val)
{
    return __sync_lock_test_and_set(addr, val);
}

static inline long atomic_swap(atomic_t *addr, long val)
{
    return __sync_lock_test_and_set((long*)addr, val);
}

static inline uint16_t atomic_swap_u16(uint16_t *addr, uint16_t val)
{
    return __sync_lock_test_and_set(addr, val);
}

static inline uint32_t atomic_swap_u32(uint32_t *addr, uint32_t val)
{
    return __sync_lock_test_and_set(addr, val);
}

static inline long atomic_read(atomic_t *number)
{
    long val;
    asm volatile("mov %1,%0" : "=r"(val) : "m"(*number));
    return val;
}

static inline void atomic_set(atomic_t *number, long val)
{
    asm volatile("mov %1,%0" : "=m"(*number) : "r"(val));
}

static inline bool atomic_cas(atomic_t *addr, long exp_val, long new_val)
{
    return __sync_bool_compare_and_swap(addr, exp_val, new_val);
}

static inline bool atomic_cas_u16(uint16_t *addr, uint16_t exp_val,
                                  uint16_t new_val)
{
    return __sync_bool_compare_and_swap(addr, exp_val, new_val);
}

static inline bool atomic_cas_u32(uint32_t *addr, uint32_t exp_val,
                                  uint32_t new_val)
{
    return __sync_bool_compare_and_swap(addr, exp_val, new_val);
}

static inline long atomic_cas_val(atomic_t *addr, long exp_val, long new_val)
{
    return (long)__sync_val_compare_and_swap(addr, exp_val, new_val);
}

/* Adds val to number, so long as number was not zero.  Returns TRUE if the
 * operation succeeded (added, not zero), returns FALSE if number is zero. */
static inline bool atomic_add_not_zero(atomic_t *number, long val)
{
    long old_num, new_num;
    do {
        old_num = atomic_read(number);
        if (!old_num)
            return false;
        new_num = old_num + val;
    } while (!atomic_cas(number, old_num, new_num));
    return true;
}

#define atomic_add(mem, value) __sync_fetch_and_add(mem, value)

#define __arch_compare_and_exchange_val_8_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgb %b2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "q" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

#define __arch_compare_and_exchange_val_16_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgw %w2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

#define __arch_compare_and_exchange_val_32_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgl %2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" (newval), "m" (*mem), "0" (oldval));	      \
     ret; })

#define __arch_compare_and_exchange_val_64_acq(mem, newval, oldval) \
  ({ __typeof (*mem) ret;						      \
     __asm __volatile ("lock;" "cmpxchgq %q2, %1"			      \
		       : "=a" (ret), "=m" (*mem)			      \
		       : "r" ((long) (newval)), "m" (*mem),		      \
			 "0" ((long) (oldval)));			      \
     ret; })


/* Note that we need no lock prefix.  */
#define atomic_exchange_acq(mem, newvalue) \
  ({ __typeof (*mem) result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("xchgb %b0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("xchgw %w0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("xchgl %0, %1"					      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else								      \
       __asm __volatile ("xchgq %q0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" ((long) (newvalue)), "m" (*mem));	      \
     result; })


#define atomic_exchange_and_add(mem, value) \
  ({ __typeof (*mem) result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "xaddb %b0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "xaddw %w0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "xaddl %0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (value), "m" (*mem));			      \
     else								      \
       __asm __volatile ("lock;" "xaddq %q0, %1"			      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" ((long) (value)), "m" (*mem));		      \
     result; })

#define atomic_add_negative(mem, value) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "addb %b2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "addw %w2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "addl %2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else								      \
       __asm __volatile ("lock;" "addq %q2, %0; sets %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" ((long) (value)), "m" (*mem));		      \
     __result; })


#define atomic_add_zero(mem, value) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "addb %b2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "addw %w2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "addl %2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" (value), "m" (*mem));			      \
     else								      \
       __asm __volatile ("lock;" "addq %q2, %0; setz %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "ir" ((long) (value)), "m" (*mem));		      \
     __result; })


#define atomic_increment_and_test(mem) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "incb %b0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "incw %w0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "incl %0; sete %1"			      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else								      \
       __asm __volatile ("lock;" "incq %q0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     __result; })


#define atomic_decrement_and_test(mem) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "decb %b0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "decw %w0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "decl %0; sete %1"			      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     else								      \
       __asm __volatile ("lock;" "decq %q0; sete %1"		      \
			 : "=m" (*mem), "=qm" (__result)		      \
			 : "m" (*mem));					      \
     __result; })


#define atomic_bit_set(mem, bit) \
  (void) ({ if (sizeof (*mem) == 1)					      \
	      __asm __volatile ("lock;" "orb %b2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "ir" (1L << (bit)));	      \
	    else if (sizeof (*mem) == 2)				      \
	      __asm __volatile ("lock;" "orw %w2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "ir" (1L << (bit)));	      \
	    else if (sizeof (*mem) == 4)				      \
	      __asm __volatile ("lock;" "orl %2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "ir" (1L << (bit)));	      \
	    else if (__builtin_constant_p (bit) && (bit) < 32)		      \
	      __asm __volatile ("lock;" "orq %2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "i" (1L << (bit)));	      \
	    else							      \
	      __asm __volatile ("lock;" "orq %q2, %0"		      \
				: "=m" (*mem)				      \
				: "m" (*mem), "r" (1UL << (bit)));	      \
	    })


#define atomic_bit_test_set(mem, bit) \
  ({ unsigned char __result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("lock;" "btsb %3, %1; setc %0" 		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("lock;" "btsw %3, %1; setc %0" 		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("lock;" "btsl %3, %1; setc %0" 	   	      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     else							      	      \
       __asm __volatile ("lock;" "btsq %3, %1; setc %0" 		      \
			 : "=q" (__result), "=m" (*mem)			      \
			 : "m" (*mem), "ir" (bit));			      \
     __result; })


#ifdef __cplusplus
}
#endif

#endif /* ATOMIC_H */

