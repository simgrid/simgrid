/* Declarations for `mmalloc' and friends. */

/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish. (fnf@cygnus.com) */

#ifndef __MMPRIVATE_H
#define __MMPRIVATE_H 1

#include <xbt/base.h>
#include <xbt/misc.h>

#include "src/internal_config.h"
#include "xbt/xbt_os_thread.h"
#include "xbt/mmalloc.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include "xbt/swag.h"

#include <pthread.h>
#include <stdint.h>

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#else
#  ifndef CHAR_BIT
#    define CHAR_BIT 8
#  endif
#endif

#define MMALLOC_MAGIC    "mmalloc"       /* Mapped file magic number */
#define MMALLOC_MAGIC_SIZE  8       /* Size of magic number buf */
#define MMALLOC_VERSION    2       /* Current mmalloc version */

/* The allocator divides the heap into blocks of fixed size; large
   requests receive one or more whole blocks, and small requests
   receive a fragment of a block.  Fragment sizes are powers of two,
   and all fragments of a block are the same size.  When all the
   fragments in a block have been freed, the block itself is freed.

   FIXME: we are not targeting 16bits machines anymore; update values */

#define INT_BIT    (CHAR_BIT * sizeof(int))
#define BLOCKLOG  (INT_BIT > 16 ? 12 : 9)
#define BLOCKSIZE  ((unsigned int) 1 << BLOCKLOG)
#define BLOCKIFY(SIZE)  (((SIZE) + BLOCKSIZE - 1) / BLOCKSIZE)

/* We keep fragment-specific meta-data for introspection purposes, and these
 * information are kept in fixed lenght arrays. Here is the computation of
 * that size.
 *
 * Never make SMALLEST_POSSIBLE_MALLOC smaller than sizeof(list) because we
 * need to enlist the free fragments.
 */

//#define SMALLEST_POSSIBLE_MALLOC (sizeof(struct list))
#define SMALLEST_POSSIBLE_MALLOC (16*sizeof(struct list))
#define MAX_FRAGMENT_PER_BLOCK (BLOCKSIZE / SMALLEST_POSSIBLE_MALLOC)

/* The difference between two pointers is a signed int.  On machines where
   the data addresses have the high bit set, we need to ensure that the
   difference becomes an unsigned int when we are using the address as an
   integral value.  In addition, when using with the '%' operator, the
   sign of the result is machine dependent for negative values, so force
   it to be treated as an unsigned int. */

#define ADDR2UINT(addr)  ((uintptr_t) ((char*) (addr) - (char*) NULL))
#define RESIDUAL(addr,bsize) ((uintptr_t) (ADDR2UINT (addr) % (bsize)))

/* Determine the amount of memory spanned by the initial heap table
   (not an absolute limit).  */

#define HEAP    (INT_BIT > 16 ? 4194304 : 65536)

/* Number of contiguous free blocks allowed to build up at the end of
   memory before they will be returned to the system.
   FIXME: this is not used anymore: we never return memory to the system. */
#define FINAL_FREE_BLOCKS  8

/* Where to start searching the free list when looking for new memory.
   The two possible values are 0 and heapindex.  Starting at 0 seems
   to reduce total memory usage, while starting at heapindex seems to
   run faster.  */

#define MALLOC_SEARCH_START  mdp -> heapindex

/* Address to block number and vice versa.  */

#define BLOCK(A) (((char*) (A) - (char*) mdp -> heapbase) / BLOCKSIZE + 1)

#define ADDRESS(B) ((void*) (((ADDR2UINT(B)) - 1) * BLOCKSIZE + (char*) mdp -> heapbase))

SG_BEGIN_DECL()

/* Doubly linked lists of free fragments.  */
struct list {
  struct list *next;
  struct list *prev;
};

/* Statistics available to the user. */
struct mstats
{
  size_t bytes_total;    /* Total size of the heap. */
  size_t chunks_used;    /* Chunks allocated by the user. */
  size_t bytes_used;    /* Byte total of user-allocated chunks. */
  size_t chunks_free;    /* Chunks in the free list. */
  size_t bytes_free;    /* Byte total of chunks in the free list. */
};

typedef struct s_heap_area{
  int valid;
  int block;
  int fragment;
}s_heap_area_t, *heap_area_t;

typedef struct s_heap_area_pair{
  int block1;
  int fragment1;
  int block2;
  int fragment2;
}s_heap_area_pair_t, *heap_area_pair_t;

#define MMALLOC_TYPE_HEAPINFO (-2)
#define MMALLOC_TYPE_FREE (-1)
#define MMALLOC_TYPE_UNFRAGMENTED 0
/* >0 values are fragmented blocks */

/* Data structure giving per-block information.
 *
 * There is one such structure in the mdp->heapinfo array per block used in that heap,
 *    the array index is the block number.
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
 */
typedef struct {
  s_xbt_swag_hookup_t freehook; /* to register this block as having empty frags when needed */
  int type; /*  0: busy large block
                >0: busy fragmented (fragments of size 2^type bytes)
                <0: free block */
  
  union {
    /* Heap information for a busy block.  */
    struct {
      size_t nfree;               /* Free fragments in a fragmented block.  */
      ssize_t frag_size[MAX_FRAGMENT_PER_BLOCK];
      //void *bt[MAX_FRAGMENT_PER_BLOCK][XBT_BACKTRACE_SIZE]; /* Where it was malloced (or realloced lastly) */
      int ignore[MAX_FRAGMENT_PER_BLOCK];
    } busy_frag;
    struct {
      size_t size; /* Size (in blocks) of a large cluster.  */
      size_t busy_size; /* Actually used space, in bytes */
      //void *bt[XBT_BACKTRACE_SIZE]; /* Where it was malloced (or realloced lastly) */
      //int bt_size;
      int ignore;
    } busy_block;
    /* Heap information for a free block (that may be the first of a free cluster).  */
    struct {
      size_t size;                /* Size (in blocks) of a free cluster.  */
      size_t next;                /* Index of next free cluster.  */
      size_t prev;                /* Index of previous free cluster.  */
    } free_block;
  };
} malloc_info;

/** @brief Descriptor of a mmalloc area
 *
 * Internal structure that defines the format of the malloc-descriptor.
 * This gets written to the base address of the region that mmalloc is
 * managing, and thus also becomes the file header for the mapped file,
 * if such a file exists.
 * */
struct mdesc {

  /** @brief Mutex locking the access to the heap */
  pthread_mutex_t mutex;

  /** @brief Number of processes that attached the heap */
  unsigned int refcount;

  /** @brief Chained lists of mdescs */
  struct mdesc *next_mdesc;

  /** @brief The "magic number" for an mmalloc file. */
  char magic[MMALLOC_MAGIC_SIZE];

  /** @brief The size in bytes of this structure
   *
   * Used as a sanity check when reusing a previously created mapped file.
   * */
  unsigned int headersize;

  /** @brief Version number of the mmalloc package that created this file. */
  unsigned char version;

  unsigned int options;

  /** @brief Some flag bits to keep track of various internal things. */
  unsigned int flags;

  /** @brief Number of info entries.  */
  size_t heapsize;

  /** @brief Pointer to first block of the heap (base of the first block).  */
  void *heapbase;

  /** @brief Current search index for the heap table.
   *
   *  Search index in the info table.
   */
  size_t heapindex;

  /** @brief Limit of valid info table indices.  */
  size_t heaplimit;

  /** @brief Block information table.
   *
   * Table indexed by block number giving per-block information.
   */
  malloc_info *heapinfo;

  /* @brief List of all blocks containing free fragments of a given size.
   *
   * The array indice is the log2 of requested size.
   * Actually only the sizes 8->11 seem to be used, but who cares? */
  s_xbt_swag_t fraghead[BLOCKLOG];

  /* @brief Base address of the memory region for this malloc heap
   *
   * This is the location where the bookkeeping data for mmap and
   * for malloc begins.
   */
  void *base;

  /** @brief End of memory in use
   *
   *  Some memory might be already mapped by the OS but not used
   *  by the heap.
   * */
  void *breakval;

  /** @brief End of the current memory region for this malloc heap.
   *
   *  This is the first location past the end of mapped memory.
   *
   *  Compared to breakval, this value is rounded to the next memory page.
   */
  void *top;

  /** @brief Open file descriptor for the file to which this malloc heap is mapped
   *
   * If this value is negative, MAP_ANONYMOUS memory is used.
   *
   * Also note that it may change each time the region is mapped and unmapped. */
  int fd;

  /* @brief Instrumentation */
  struct mstats heapstats;

};

/* Bits to look at in the malloc descriptor flags word */

#define MMALLOC_DEVZERO    (1 << 0)        /* Have mapped to /dev/zero */
#define MMALLOC_ANONYMOUS (1 << 1)      /* Use anonymous mapping */
#define MMALLOC_INITIALIZED  (1 << 2)        /* Initialized mmalloc */

/* A default malloc descriptor for the single sbrk() managed region. */

XBT_PUBLIC_DATA( struct mdesc ) *__mmalloc_default_mdp;

/* Remap a mmalloc region that was previously mapped. */

XBT_PUBLIC( void *)__mmalloc_remap_core(xbt_mheap_t mdp);

XBT_PUBLIC( void *)mmorecore(struct mdesc *mdp, ssize_t size);

/** Thread-safety (if the mutex is already created)
 *
 * This is mandatory in the case where the user runs a parallel simulation
 * in a model-checking enabled tree. Without this protection, our malloc
 * implementation will not like multi-threading AT ALL.
 */
#define LOCK(mdp) pthread_mutex_lock(&mdp->mutex)
#define UNLOCK(mdp) pthread_mutex_unlock(&mdp->mutex)

static inline void  mmalloc_paranoia(struct mdesc *mdp){

  /* nothing to fear for no */

}

static inline int mmalloc_get_increment(malloc_info* heapinfo) {
  if (heapinfo->type < 0) {
    return heapinfo->free_block.size;
  } else if (heapinfo->type == 0) {
    return heapinfo->busy_block.size;
  } else {
    return 1;
  }
}

XBT_PRIVATE void mmcheck(xbt_mheap_t heap);

XBT_PRIVATE int malloc_use_mmalloc(void);

XBT_PRIVATE int mmalloc_exec_using_mm(int argc, const char** argv);
XBT_PRIVATE void mmalloc_ensure_using_mm(int argc, const char** argv);

XBT_PRIVATE size_t mmalloc_get_bytes_used_remote(size_t heaplimit, const malloc_info* heapinfo);

SG_END_DECL()

#endif                          /* __MMPRIVATE_H */
