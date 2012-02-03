/* Declarations for `mmalloc' and friends.
   Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish. (fnf@cygnus.com) */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef __MMPRIVATE_H
#define __MMPRIVATE_H 1

#include "portable.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/mmalloc.h"
#include "xbt/ex.h"
#include <semaphore.h>

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#else
#  ifndef CHAR_BIT
#    define CHAR_BIT 8
#  endif
#endif

#define MMALLOC_MAGIC		"mmalloc"       /* Mapped file magic number */
#define MMALLOC_MAGIC_SIZE	8       /* Size of magic number buf */
#define MMALLOC_VERSION		1       /* Current mmalloc version */

/* The allocator divides the heap into blocks of fixed size; large
   requests receive one or more whole blocks, and small requests
   receive a fragment of a block.  Fragment sizes are powers of two,
   and all fragments of a block are the same size.  When all the
   fragments in a block have been freed, the block itself is freed.

   FIXME: we are not targeting 16bits machines anymore; update values */

#define INT_BIT		(CHAR_BIT * sizeof(int))
#define BLOCKLOG	(INT_BIT > 16 ? 12 : 9)
#define BLOCKSIZE	((unsigned int) 1 << BLOCKLOG)
#define BLOCKIFY(SIZE)	(((SIZE) + BLOCKSIZE - 1) / BLOCKSIZE)

/* We keep fragment-specific meta-data for introspection purposes, and these
 * information are kept in fixed lenght arrays. Here is the computation of
 * that size.
 *
 * Never make SMALLEST_POSSIBLE_MALLOC smaller than sizeof(list) because we
 * need to enlist the free fragments.
 */

#define SMALLEST_POSSIBLE_MALLOC (sizeof(struct list))
#define MAX_FRAGMENT_PER_BLOCK (BLOCKSIZE / SMALLEST_POSSIBLE_MALLOC)

/* The difference between two pointers is a signed int.  On machines where
   the data addresses have the high bit set, we need to ensure that the
   difference becomes an unsigned int when we are using the address as an
   integral value.  In addition, when using with the '%' operator, the
   sign of the result is machine dependent for negative values, so force
   it to be treated as an unsigned int. */

#define ADDR2UINT(addr)	((unsigned int) ((char*) (addr) - (char*) NULL))
#define RESIDUAL(addr,bsize) ((unsigned int) (ADDR2UINT (addr) % (bsize)))

/* Determine the amount of memory spanned by the initial heap table
   (not an absolute limit).  */

#define HEAP		(INT_BIT > 16 ? 4194304 : 65536)

/* Number of contiguous free blocks allowed to build up at the end of
   memory before they will be returned to the system.
   FIXME: this is not used anymore: we never return memory to the system. */
#define FINAL_FREE_BLOCKS	8

/* Where to start searching the free list when looking for new memory.
   The two possible values are 0 and heapindex.  Starting at 0 seems
   to reduce total memory usage, while starting at heapindex seems to
   run faster.  */

#define MALLOC_SEARCH_START	mdp -> heapindex

/* Address to block number and vice versa.  */

#define BLOCK(A) (((char*) (A) - (char*) mdp -> heapbase) / BLOCKSIZE + 1)

#define ADDRESS(B) ((void*) (((ADDR2UINT(B)) - 1) * BLOCKSIZE + (char*) mdp -> heapbase))

const char *xbt_thread_self_name(void);

/* Doubly linked lists of free fragments.  */
struct list {
	struct list *next;
	struct list *prev;
};

/* Data structure giving per-block information.
 *
 * There is one such structure in the mdp->heapinfo array,
 * that is addressed by block number.
 *
 * There is several types of blocks in memory:
 *  - full busy blocks: used when we are asked to malloc a block which size is > BLOCKSIZE/2
 *    In this situation, the full block is given to the malloc.
 *
 *  - fragmented busy blocks: when asked for smaller amount of memory.
 *    Fragment sizes are only power of 2. When looking for such a free fragment,
 *    we get one from mdp->fraghead (that contains a linked list of blocks fragmented at that
 *    size and containing a free fragment), or we get a fresh block that we fragment.
 *
 *  - free blocks are grouped by clusters, that are chained together.
 *    When looking for free blocks, we traverse the mdp->heapinfo looking
 *    for a cluster of free blocks that would be large enough.
 *
 *    The size of the cluster is only to be trusted in the first block of the cluster, not in the middle blocks.
 *
 * The type field is consistently updated for every blocks, even within clusters of blocks.
 * You can crawl the array and rely on that value.
 *
 * TODO:
 *  - add an indication of the requested size in each fragment, similarly to busy_block.busy_size
 *  - make room to store the backtrace of where the blocks and fragment were malloced, too.
 */
typedef struct {
	int type; /*  0: busy large block
		         >0: busy fragmented (fragments of size 2^type bytes)
		         <0: free block */
	union {
	/* Heap information for a busy block.  */
		struct {
			size_t nfree;           /* Free fragments in a fragmented block.  */
			size_t first;           /* First free fragment of the block.  */
			unsigned short frag_size[MAX_FRAGMENT_PER_BLOCK];
		} busy_frag;
		struct {
			size_t size; /* Size (in blocks) of a large cluster.  */
			size_t busy_size; /* Actually used space, in bytes */
		} busy_block;
		/* Heap information for a free block (that may be the first of a free cluster).  */
		struct {
			size_t size;                /* Size (in blocks) of a free cluster.  */
			size_t next;                /* Index of next free cluster.  */
			size_t prev;                /* Index of previous free cluster.  */
		} free_block;
	};
} malloc_info;

/* Internal structure that defines the format of the malloc-descriptor.
   This gets written to the base address of the region that mmalloc is
   managing, and thus also becomes the file header for the mapped file,
   if such a file exists. */

struct mdesc {

	/* Semaphore locking the access to the heap */
	sem_t sem;

	/* Number of processes that attached the heap */
	unsigned int refcount;

	/* Chained lists of mdescs */
	struct mdesc *next_mdesc;

	/* The "magic number" for an mmalloc file. */
	char magic[MMALLOC_MAGIC_SIZE];

	/* The size in bytes of this structure, used as a sanity check when reusing
     a previously created mapped file. */
	unsigned int headersize;

	/* The version number of the mmalloc package that created this file. */
	unsigned char version;

	/* Some flag bits to keep track of various internal things. */
	unsigned int flags;

	/* Number of info entries.  */
	size_t heapsize;

	/* Pointer to first block of the heap (base of the first block).  */
	void *heapbase;

	/* Current search index for the heap table.  */
	/* Search index in the info table.  */
	size_t heapindex;

	/* Limit of valid info table indices.  */
	size_t heaplimit;

	/* Block information table.
     Allocated with malign/mfree (not mmalloc/mfree).  */
	/* Table indexed by block number giving per-block information.  */
	malloc_info *heapinfo;

	/* List of all blocks containing free fragments of this size. The array indice is the log2 of requested size */
	struct list fraghead[BLOCKLOG];

	/* The base address of the memory region for this malloc heap.  This
     is the location where the bookkeeping data for mmap and for malloc
     begins. */

	void *base;

	/* The current location in the memory region for this malloc heap which
     represents the end of memory in use. */

	void *breakval;

	/* The end of the current memory region for this malloc heap.  This is
     the first location past the end of mapped memory. */

	void *top;

	/* Open file descriptor for the file to which this malloc heap is mapped.
     This will always be a valid file descriptor, since /dev/zero is used
     by default if no open file is supplied by the client.  Also note that
     it may change each time the region is mapped and unmapped. */

	int fd;

};

int mmalloc_compare_mdesc(struct mdesc *mdp1, struct mdesc *mdp2, void *std_heap_addr);

void mmalloc_display_info(void *h);

/* Bits to look at in the malloc descriptor flags word */

#define MMALLOC_DEVZERO		(1 << 0)        /* Have mapped to /dev/zero */
#define MMALLOC_ANONYMOUS (1 << 1)      /* Use anonymous mapping */
#define MMALLOC_INITIALIZED	(1 << 2)        /* Initialized mmalloc */

/* A default malloc descriptor for the single sbrk() managed region. */

extern struct mdesc *__mmalloc_default_mdp;

/* Remap a mmalloc region that was previously mapped. */

extern void *__mmalloc_remap_core(xbt_mheap_t mdp);

/*  Get core for the memory region specified by MDP, using SIZE as the
    amount to either add to or subtract from the existing region.  Works
    like sbrk(), but using mmap(). */
extern void *mmorecore(struct mdesc *mdp, int size);

/* Thread-safety (if the sem is already created) FIXME: KILLIT*/
#define LOCK(mdp)                                        \
		sem_wait(&mdp->sem)

#define UNLOCK(mdp)                                        \
		sem_post(&mdp->sem)

#endif                          /* __MMPRIVATE_H */
