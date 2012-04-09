/*
 * Copyright (c) 2011 The Regents of the University of California
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

#include <assert.h>
#include <stdio.h>

#include "parlib.h"
#include "vcore.h"
#include "mcs.h"

static mcs_lock_t lock = MCS_LOCK_INIT;

int main(int argc, char** argv)
{
	/* Make sure we are not in vcore context */
	assert(!in_vcore_context());

	/* Init the vcore system */
	assert(!vcore_lib_init());

	/* Request as many as the system will give us */
	vcore_request(max_vcores());
	vcore_yield(false);
	assert(0);
}

int global_count = 0;

void vcore_entry(void)
{
	/* Make sure we are in vcore context */
	assert(in_vcore_context());

	if(vcore_saved_ucontext) {
      setcontext(vcore_saved_ucontext);
	}

	/* Test the locks forever.  Should never dead lock */
	printf("Begin infinite loop on vcore %d\n", vcore_id());
	uint32_t vcoreid = vcore_id();
	mcs_lock_qnode_t qnode = {0};
	while(1) {
		memset(&qnode, 0, sizeof(mcs_lock_qnode_t));
		mcs_lock_lock(&lock, &qnode);
		printf("global_count: %d, on vcore %d\n", global_count++, vcoreid);
		mcs_lock_unlock(&lock, &qnode);
	}
	assert(0);
}

