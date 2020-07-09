/* Memory allocator `malloc'. */

/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish for mmap'd version. */

#include <string.h>             /* Prototypes for memcpy, memmove, memset, etc */
#include <stdio.h>
#include "mmprivate.h"

/* Prototypes for local functions */

static void initialize(xbt_mheap_t mdp);
static void *register_morecore(xbt_mheap_t mdp, size_t size);
static void *align(xbt_mheap_t mdp, size_t size);

/* Allocation aligned on block boundary.
 *
 * It never returns NULL, but dies verbosely on error.
 */
static void *align(struct mdesc *mdp, size_t size)
{
  void *result;
  unsigned long int adj;

  result = mmorecore(mdp, size);

  /* if this reservation does not fill up the last block of our resa,
   * complete the reservation by also asking for the full latest block.
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

/** Initialize heapinfo about the heapinfo pages :)
 *
 */
static void initialize_heapinfo_heapinfo(const s_xbt_mheap_t* mdp)
{
  // Update heapinfo about the heapinfo pages (!):
  xbt_assert((uintptr_t) mdp->heapinfo % BLOCKSIZE == 0);
  size_t block   = BLOCK(mdp->heapinfo);
  size_t nblocks = mdp->heapsize * sizeof(malloc_info) / BLOCKSIZE;
  // Mark them as free:
  for (size_t j=0; j!=nblocks; ++j) {
    mdp->heapinfo[block+j].type = MMALLOC_TYPE_FREE;
    mdp->heapinfo[block+j].free_block.size = 0;
    mdp->heapinfo[block+j].free_block.next = 0;
    mdp->heapinfo[block+j].free_block.prev = 0;
  }
  mdp->heapinfo[block].free_block.size = nblocks;
}

/* Finish the initialization of the mheap. If we want to inline it
 * properly, we need to make the align function publicly visible, too  */
static void initialize(xbt_mheap_t mdp)
{
  int i;
  malloc_info mi; /* to compute the offset of the swag hook */

  // Update mdp meta-data:
  mdp->heapsize = HEAP / BLOCKSIZE;
  mdp->heapinfo = (malloc_info *)
    align(mdp, mdp->heapsize * sizeof(malloc_info));
  mdp->heapbase = (void *) mdp->heapinfo;
  mdp->flags |= MMALLOC_INITIALIZED;

  // Update root heapinfo:
  memset((void *) mdp->heapinfo, 0, mdp->heapsize * sizeof(malloc_info));
  mdp->heapinfo[0].type = MMALLOC_TYPE_FREE;
  mdp->heapinfo[0].free_block.size = 0;
  mdp->heapinfo[0].free_block.next = mdp->heapinfo[0].free_block.prev = 0;
  mdp->heapindex = 0;

  initialize_heapinfo_heapinfo(mdp);

  for (i=0;i<BLOCKLOG;i++) {
      xbt_swag_init(&(mdp->fraghead[i]),
                    xbt_swag_offset(mi, freehook));
  }
}

static inline void update_hook(void **a, size_t offset)
{
  if (*a)
    *a = (char*)*a + offset;
}

/* Get neatly aligned memory from the low level layers, and register it
 * into the heap info table as necessary. */
static void *register_morecore(struct mdesc *mdp, size_t size)
{
  void* result = align(mdp, size); // Never returns NULL

  /* Check if we need to grow the info table (in a multiplicative manner)  */
  if ((size_t) BLOCK((char *) result + size) > mdp->heapsize) {
    int it;

    size_t newsize = mdp->heapsize;
    while ((size_t) BLOCK((char *) result + size) > newsize)
      newsize *= 2;

    /* Copy old info into new location */
    malloc_info* oldinfo = mdp->heapinfo;
    malloc_info* newinfo = (malloc_info*)align(mdp, newsize * sizeof(malloc_info));
    memcpy(newinfo, oldinfo, mdp->heapsize * sizeof(malloc_info));

    /* Initialize the new blockinfo : */
    memset((char*) newinfo + mdp->heapsize * sizeof(malloc_info), 0,
      (newsize - mdp->heapsize)* sizeof(malloc_info));

    /* Update the swag of busy blocks containing free fragments by applying the offset to all swag_hooks. Yeah. My hand is right in the fan and I still type */
    size_t offset=((char*)newinfo)-((char*)oldinfo);

    for (int i = 1 /*first element of heapinfo describes the mdesc area*/; i < mdp->heaplimit; i++) {
      update_hook(&newinfo[i].freehook.next, offset);
      update_hook(&newinfo[i].freehook.prev, offset);
    }
    // also update the starting points of the swag
    for (int i = 0; i < BLOCKLOG; i++) {
      update_hook(&mdp->fraghead[i].head, offset);
      update_hook(&mdp->fraghead[i].tail, offset);
    }
    mdp->heapinfo = newinfo;

    /* mark the space previously occupied by the block info as free by first marking it
     * as occupied in the regular way, and then freing it */
    for (it=0; it<BLOCKIFY(mdp->heapsize * sizeof(malloc_info)); it++){
      newinfo[BLOCK(oldinfo)+it].type = MMALLOC_TYPE_UNFRAGMENTED;
      newinfo[BLOCK(oldinfo)+it].busy_block.ignore = 0;
    }

    newinfo[BLOCK(oldinfo)].busy_block.size = BLOCKIFY(mdp->heapsize * sizeof(malloc_info));
    newinfo[BLOCK(oldinfo)].busy_block.busy_size = size;
    mfree(mdp, (void *) oldinfo);
    mdp->heapsize = newsize;

    initialize_heapinfo_heapinfo(mdp);
  }

  mdp->heaplimit = BLOCK((char *) result + size);
  return (result);
}

/* Allocate memory from the heap.  */
void *mmalloc(xbt_mheap_t mdp, size_t size) {
  void *res= mmalloc_no_memset(mdp,size);
  if (mdp->options & XBT_MHEAP_OPTION_MEMSET) {
    memset(res,0,size);
  }
  return res;
}

static void mmalloc_mark_used(xbt_mheap_t mdp, size_t block, size_t nblocks, size_t requested_size)
{
  for (int it = 0; it < nblocks; it++) {
    mdp->heapinfo[block + it].type                 = MMALLOC_TYPE_UNFRAGMENTED;
    mdp->heapinfo[block + it].busy_block.busy_size = 0;
    mdp->heapinfo[block + it].busy_block.ignore    = 0;
    mdp->heapinfo[block + it].busy_block.size      = 0;
  }
  mdp->heapinfo[block].busy_block.size      = nblocks;
  mdp->heapinfo[block].busy_block.busy_size = requested_size;
  mdp->heapstats.chunks_used++;
  mdp->heapstats.bytes_used += nblocks * BLOCKSIZE;
}

/* Splitting mmalloc this way is mandated by a trick in mrealloc, that gives
   back the memory of big blocks to the system before reallocating them: we don't
   want to loose the beginning of the area when this happens */
void *mmalloc_no_memset(xbt_mheap_t mdp, size_t size)
{
  void *result;
  size_t block;

  size_t requested_size = size; // The amount of memory requested by user, for real

  /* Work even if the user was stupid enough to ask a ridiculously small block (even 0-length),
   *    ie return a valid block that can be realloced and freed.
   * glibc malloc does not use this trick but return a constant pointer, but we need to enlist the free fragments later on.
   */
  if (size < SMALLEST_POSSIBLE_MALLOC)
    size = SMALLEST_POSSIBLE_MALLOC;

  if (!(mdp->flags & MMALLOC_INITIALIZED))
    initialize(mdp);

  /* Determine the allocation policy based on the request size.  */
  if (size <= BLOCKSIZE / 2) {
    /* Small allocation to receive a fragment of a block.
       Determine the logarithm to base two of the fragment size. */
    size_t log = 1;
    --size;
    while ((size /= 2) != 0) {
      ++log;
    }

    /* Look in the fragment lists for a free fragment of the desired size. */
    if (xbt_swag_size(&mdp->fraghead[log])>0) {
      /* There are free fragments of this size; Get one of them and prepare to return it.
         Update the block's nfree and if no other free fragment, get out of the swag. */

      /* search a fragment that I could return as a result */
      malloc_info *candidate_info = xbt_swag_getFirst(&mdp->fraghead[log]);
      size_t candidate_block = (candidate_info - &(mdp->heapinfo[0]));
      size_t candidate_frag;
      for (candidate_frag=0;candidate_frag<(size_t) (BLOCKSIZE >> log);candidate_frag++)
        if (candidate_info->busy_frag.frag_size[candidate_frag] == -1)
          break;
      xbt_assert(candidate_frag < (size_t) (BLOCKSIZE >> log),
          "Block %zu was registered as containing free fragments of type %zu, but I can't find any",candidate_block,log);

      result = (void*) (((char*)ADDRESS(candidate_block)) + (candidate_frag << log));

      /* Remove this fragment from the list of free guys */
      candidate_info->busy_frag.nfree--;
      if (candidate_info->busy_frag.nfree == 0) {
        xbt_swag_remove(candidate_info,&mdp->fraghead[log]);
      }

      /* Update our metadata about this fragment */
      candidate_info->busy_frag.frag_size[candidate_frag] = requested_size;
      candidate_info->busy_frag.ignore[candidate_frag] = 0;

      /* Update the statistics.  */
      mdp -> heapstats.chunks_used++;
      mdp -> heapstats.bytes_used += 1 << log;
      mdp -> heapstats.chunks_free--;
      mdp -> heapstats.bytes_free -= 1 << log;

    } else {
      /* No free fragments of the desired size, so get a new block
         and break it into fragments, returning the first.  */

      result = mmalloc(mdp, BLOCKSIZE); // does not return NULL
      block = BLOCK(result);

      mdp->heapinfo[block].type = (int)log;
      /* Link all fragments but the first as free, and add the block to the swag of blocks containing free frags  */
      size_t i;
      for (i = 1; i < (size_t) (BLOCKSIZE >> log); ++i) {
        mdp->heapinfo[block].busy_frag.frag_size[i] = -1;
        mdp->heapinfo[block].busy_frag.ignore[i] = 0;
      }
      mdp->heapinfo[block].busy_frag.nfree = i - 1;
      mdp->heapinfo[block].freehook.prev = NULL;
      mdp->heapinfo[block].freehook.next = NULL;

      xbt_swag_insert(&mdp->heapinfo[block], &(mdp->fraghead[log]));

      /* mark the fragment returned as busy */
      mdp->heapinfo[block].busy_frag.frag_size[0] = requested_size;
      mdp->heapinfo[block].busy_frag.ignore[0] = 0;

      /* update stats */
      mdp -> heapstats.chunks_free += (BLOCKSIZE >> log) - 1;
      mdp -> heapstats.bytes_free += BLOCKSIZE - (1 << log);
      mdp -> heapstats.bytes_used -= BLOCKSIZE - (1 << log);
    }
  } else {
    /* Large allocation to receive one or more blocks.
       Search the free list in a circle starting at the last place visited.
       If we loop completely around without finding a large enough
       space we will have to get more memory from the system.  */
    size_t blocks = BLOCKIFY(size);
    size_t start  = MALLOC_SEARCH_START;
    block         = MALLOC_SEARCH_START;
    while (mdp->heapinfo[block].free_block.size < blocks) {
      if (mdp->heapinfo[block].type >=0) { // Don't trust xbt_die and friends in malloc-level library, you fool!
        fprintf(stderr,
                "Internal error: found a free block not marked as such (block=%zu type=%d). Please report this bug.\n",
                block, mdp->heapinfo[block].type);
        abort();
      }

      block = mdp->heapinfo[block].free_block.next;
      if (block == start) {
        /* Need to get more from the system.  Check to see if
           the new core will be contiguous with the final free
           block; if so we don't need to get as much.  */
        block = mdp->heapinfo[0].free_block.prev;
        size_t lastblocks = mdp->heapinfo[block].free_block.size;
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

        block = BLOCK(result);
        mmalloc_mark_used(mdp, block, blocks, requested_size);

        return result;
      }
      /* Need large block(s), but found some in the existing heap */
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

    mmalloc_mark_used(mdp, block, blocks, requested_size);
    mdp -> heapstats.bytes_free -= blocks * BLOCKSIZE;

  }

  return (result);
}
