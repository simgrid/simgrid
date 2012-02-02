/* Memory allocator `malloc'.
   Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish for mmap'd version. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string.h>             /* Prototypes for memcpy, memmove, memset, etc */
#include <stdio.h>
#include "mmprivate.h"

/* Prototypes for local functions */

static int initialize(xbt_mheap_t mdp);
static void *register_morecore(xbt_mheap_t mdp, size_t size);
static void *align(xbt_mheap_t mdp, size_t size);

/* Allocation aligned on block boundary */
static void *align(struct mdesc *mdp, size_t size)
{
  void *result;
  unsigned long int adj;

  result = mmorecore(mdp, size);

  /* if this reservation does not fill up the last block of our resa,
   * complete the reservation by also asking for the full lastest block.
   *
   * Also, the returned block is aligned to the end of block (but I've
   * no fucking idea of why, actually -- http://abstrusegoose.com/432 --
   * but not doing so seems to lead to issues).
   */
  adj = RESIDUAL(result, BLOCKSIZE);
  if (adj != 0) {
    adj = BLOCKSIZE - adj;
    mmorecore(mdp, adj);
    result = (char *) result + adj;
  }
  return (result);
}

/* Finish the initialization of the mheap. If we want to inline it
 * properly, we need to make the align function publicly visible, too  */
static int initialize(xbt_mheap_t mdp)
{
  mdp->heapsize = HEAP / BLOCKSIZE;
  mdp->heapinfo = (malloc_info *)
      align(mdp, mdp->heapsize * sizeof(malloc_info));
  if (mdp->heapinfo == NULL) {
    return (0);
  }
  memset((void *) mdp->heapinfo, 0, mdp->heapsize * sizeof(malloc_info));
  mdp->heapinfo[0].type=-1;
  mdp->heapinfo[0].free_block.size = 0;
  mdp->heapinfo[0].free_block.next = mdp->heapinfo[0].free_block.prev = 0;
  mdp->heapindex = 0;
  mdp->heapbase = (void *) mdp->heapinfo;
  mdp->flags |= MMALLOC_INITIALIZED;
  return (1);
}

/* Get neatly aligned memory from the low level layers, and register it
 * into the heap info table as necessary. */
static void *register_morecore(struct mdesc *mdp, size_t size)
{
  void *result;
  malloc_info *newinfo, *oldinfo;
  size_t newsize;

  result = align(mdp, size);
  if (result == NULL) {
    return (NULL);
  }

  /* Check if we need to grow the info table (in a multiplicative manner)  */
  if ((size_t) BLOCK((char *) result + size) > mdp->heapsize) {
	int it;

    newsize = mdp->heapsize;
    while ((size_t) BLOCK((char *) result + size) > newsize)
      newsize *= 2;

    /* Copy old info into new location */
    oldinfo = mdp->heapinfo;
    newinfo = (malloc_info *) align(mdp, newsize * sizeof(malloc_info));
    memset(newinfo, 0, newsize * sizeof(malloc_info));
    memcpy(newinfo, oldinfo, mdp->heapsize * sizeof(malloc_info));
    mdp->heapinfo = newinfo;

    /* mark the space previously occupied by the block info as free by first marking it
     * as occupied in the regular way, and then freing it */
    for (it=0; it<BLOCKIFY(mdp->heapsize * sizeof(malloc_info)); it++)
    	newinfo[BLOCK(oldinfo)+it].type = 0;

    newinfo[BLOCK(oldinfo)].busy_block.size = BLOCKIFY(mdp->heapsize * sizeof(malloc_info));
    newinfo[BLOCK(oldinfo)].busy_block.busy_size = size;
    mfree(mdp, (void *) oldinfo);
    mdp->heapsize = newsize;
  }

  mdp->heaplimit = BLOCK((char *) result + size);
  return (result);
}

/* Allocate memory from the heap.  */

void *mmalloc(xbt_mheap_t mdp, size_t size)
{
  void *result;
  size_t block, blocks, lastblocks, start;
  register size_t i;
  struct list *next;
  register size_t log;
  int it;

  /* Work even if the user was stupid enough to ask a 0-byte block, ie return a valid block that can be realloced or freed
   * glibc malloc does not use this trick but return a constant pointer, but my hack is quicker to implement ;)
   */
  if (size == 0)
    size = 1;

//  printf("(%s) Mallocing %d bytes on %p (default: %p)...",xbt_thread_self_name(),size,mdp,__mmalloc_default_mdp);fflush(stdout);

  if (!(mdp->flags & MMALLOC_INITIALIZED)) {
    if (!initialize(mdp)) {
      return (NULL);
    }
  }

  if (size < sizeof(struct list)) {
    size = sizeof(struct list);
  }

  /* Determine the allocation policy based on the request size.  */
  if (size <= BLOCKSIZE / 2) {
    /* Small allocation to receive a fragment of a block.
       Determine the logarithm to base two of the fragment size. */
    log = 1;
    --size;
    while ((size /= 2) != 0) {
      ++log;
    }

    /* Look in the fragment lists for a
       free fragment of the desired size. */
    next = mdp->fraghead[log].next;
    if (next != NULL) {
      /* There are free fragments of this size.
         Pop a fragment out of the fragment list and return it.
         Update the block's nfree and first counters. */
      result = (void *) next;
      next->prev->next = next->next;
      if (next->next != NULL) {
        next->next->prev = next->prev;
      }
      block = BLOCK(result);
      if (--mdp->heapinfo[block].busy_frag.nfree != 0) {
        mdp->heapinfo[block].busy_frag.first =
            RESIDUAL(next->next, BLOCKSIZE) >> log;
      }

    } else {
      /* No free fragments of the desired size, so get a new block
         and break it into fragments, returning the first.  */
      //printf("(%s) No free fragment...",xbt_thread_self_name());
      result = mmalloc(mdp, BLOCKSIZE);
      //printf("(%s) Fragment: %p...",xbt_thread_self_name(),result);
      if (result == NULL) {
        return (NULL);
      }

      /* Link all fragments but the first into the free list.  */
      for (i = 1; i < (size_t) (BLOCKSIZE >> log); ++i) {
        next = (struct list *) ((char *) result + (i << log));
        next->next = mdp->fraghead[log].next;
        next->prev = &mdp->fraghead[log];
        next->prev->next = next;
        if (next->next != NULL) {
          next->next->prev = next;
        }
      }

      /* Initialize the nfree and first counters for this block.  */
      block = BLOCK(result);
      mdp->heapinfo[block].type = log;
      mdp->heapinfo[block].busy_frag.nfree = i - 1;
      mdp->heapinfo[block].busy_frag.first = i - 1;
    }
  } else {
    /* Large allocation to receive one or more blocks.
       Search the free list in a circle starting at the last place visited.
       If we loop completely around without finding a large enough
       space we will have to get more memory from the system.  */
    blocks = BLOCKIFY(size);
    start = block = MALLOC_SEARCH_START;
    while (mdp->heapinfo[block].free_block.size < blocks) {
    	if (mdp->heapinfo[block].type >=0) {
    		fprintf(stderr,"Internal error: found a free block not marked as such (block=%lu type=%lu). Please report this bug.\n",(unsigned long)block,(unsigned long)mdp->heapinfo[block].type);
    		abort();
    	}

      block = mdp->heapinfo[block].free_block.next;
      if (block == start) {
        /* Need to get more from the system.  Check to see if
           the new core will be contiguous with the final free
           block; if so we don't need to get as much.  */
        block = mdp->heapinfo[0].free_block.prev;
        lastblocks = mdp->heapinfo[block].free_block.size;
        if (mdp->heaplimit != 0 &&
            block + lastblocks == mdp->heaplimit &&
            mmorecore(mdp, 0) == ADDRESS(block + lastblocks) &&
            (register_morecore(mdp, (blocks - lastblocks) * BLOCKSIZE)) != NULL) {
          /* Which block we are extending (the `final free
             block' referred to above) might have changed, if
             it got combined with a freed info table.  */
          block = mdp->heapinfo[0].free_block.prev;

          mdp->heapinfo[block].free_block.size += (blocks - lastblocks);
          continue;
        }
        result = register_morecore(mdp, blocks * BLOCKSIZE);
        if (result == NULL) {
          return (NULL);
        }
        block = BLOCK(result);
        for (it=0;it<blocks;it++)
        	mdp->heapinfo[block+it].type = 0;
        mdp->heapinfo[block].busy_block.size = blocks;
	mdp->heapinfo[block].busy_block.busy_size = size;
        return (result);
      }
    }

    /* At this point we have found a suitable free list entry.
       Figure out how to remove what we need from the list. */
    result = ADDRESS(block);
    if (mdp->heapinfo[block].free_block.size > blocks) {
      /* The block we found has a bit left over,
         so relink the tail end back into the free list. */
      mdp->heapinfo[block + blocks].free_block.size
          = mdp->heapinfo[block].free_block.size - blocks;
      mdp->heapinfo[block + blocks].free_block.next
          = mdp->heapinfo[block].free_block.next;
      mdp->heapinfo[block + blocks].free_block.prev
          = mdp->heapinfo[block].free_block.prev;
      mdp->heapinfo[mdp->heapinfo[block].free_block.prev].free_block.next
          = mdp->heapinfo[mdp->heapinfo[block].free_block.next].free_block.prev
          = mdp->heapindex = block + blocks;
    } else {
      /* The block exactly matches our requirements,
         so just remove it from the list. */
      mdp->heapinfo[mdp->heapinfo[block].free_block.next].free_block.prev
          = mdp->heapinfo[block].free_block.prev;
      mdp->heapinfo[mdp->heapinfo[block].free_block.prev].free_block.next
          = mdp->heapindex = mdp->heapinfo[block].free_block.next;
    }

    for (it=0;it<blocks;it++)
    	mdp->heapinfo[block+it].type = 0;
    mdp->heapinfo[block].busy_block.size = blocks;
    mdp->heapinfo[block].busy_block.busy_size = size;
  }
  //printf("(%s) Done mallocing. Result is %p\n",xbt_thread_self_name(),result);fflush(stdout);
  return (result);
}
