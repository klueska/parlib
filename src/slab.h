/*
 * Copyright (c) 2009 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 *
 * Slab allocator, based on the SunOS 5.4 allocator paper.
 *
 * There is a list of slab_cache, which are the caches of objects of a given
 * size.  This list is sorted in order of size.  Each slab_cache has three
 * lists of slabs: full, partial, and empty.  
 *
 * For large objects, the slabs point to bufctls, which have the address
 * of their large buffers.  These slabs can consist of more than one contiguous
 * page.
 *
 * For small objects, the slabs do not use the bufctls.  Instead, they point to
 * the next free object in the slab.  The free objects themselves hold the
 * address of the next free item.  The slab structure is stored at the end of
 * the page.  There is only one page per slab.
 *
 * TODO: Note, that this is a minor pain in the ass, and worth thinking about
 * before implementing.  To keep the constructor's state valid, we can't just
 * overwrite things, so we need to add an extra 4-8 bytes per object for the
 * pointer, and then pass over that data when we return the actual object's
 * address.  This also might fuck with alignment.
 *
 * Ported directly from the Akaros kernel's slab allocator. */

#ifndef PARLIB_SLAB_H
#define PARLIB_SLAB_H

#include <sys/queue.h>
#include "spinlock.h"
#include "parlib.h"

/* Back in the day, their cutoff for "large objects" was 512B, based on
 * measurements and on not wanting more than 1/8 of internal fragmentation. */
#define NUM_BUF_PER_SLAB 8
#define SLAB_LARGE_CUTOFF (PGSIZE / NUM_BUF_PER_SLAB)

struct slab;
typedef struct slab slab_t;

typedef void (*slab_cache_ctor_t)(void *, size_t);
typedef void (*slab_cache_dtor_t)(void *, size_t);

/* Control block for buffers for large-object slabs */
struct slab_bufctl {
	TAILQ_ENTRY(slab_bufctl) link;
	void *buf_addr;
	struct slab *my_slab;
};
TAILQ_HEAD(slab_bufctl_list, slab_bufctl);

/* Slabs contain the objects.  Can be either full, partial, or empty,
 * determined by checking the number of objects busy vs total.  For large
 * slabs, the bufctl list is used to find a free buffer.  For small, the void*
 * is used instead.*/
struct slab {
	TAILQ_ENTRY(slab) link;
	size_t obj_size;
	size_t num_busy_obj;
	size_t num_total_obj;
	union {
		struct slab_bufctl_list bufctl_freelist;
		void *free_small_obj;
	};
};
TAILQ_HEAD(slab_list, slab);

/* Actual cache */
typedef struct slab_cache {
	SLIST_ENTRY(slab_cache) link;
	spinlock_t cache_lock;
	const char *name;
	size_t obj_size;
	int align;
	int flags;
	struct slab_list full_slab_list;
	struct slab_list partial_slab_list;
	struct slab_list empty_slab_list;
	slab_cache_ctor_t ctor;
	slab_cache_dtor_t dtor;
	unsigned long nr_cur_alloc;
} slab_cache_t;

/* List of all slab_caches, sorted in order of size */
SLIST_HEAD(slab_cache_list, slab_cache);
extern struct slab_cache_list slab_caches;

/* Cache management */
struct slab_cache *slab_cache_create(const char *name, size_t obj_size,
                                     int align, int flags,
                                     slab_cache_ctor_t ctor,
                                     slab_cache_dtor_t dtor);
void slab_cache_destroy(struct slab_cache *cp);
/* Front end: clients of caches use these */
void *slab_cache_alloc(struct slab_cache *cp, int flags);
void slab_cache_free(struct slab_cache *cp, void *buf);
/* Back end: internal functions */
void slab_cache_init(void);
void slab_cache_reap(struct slab_cache *cp);

/* Debug */
void print_slab_cache(struct slab_cache *kc);
void print_slab(struct slab *slab);

#endif /* PARLIB_SLAB_H */
