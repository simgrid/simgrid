/* Free a block of memory allocated by `mmalloc'. */

/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish.  (fnf@cygnus.com) */

#include "mmprivate.h"
#include "xbt/ex.h"
#include "mc/mc.h"

/* Return memory to the heap.
   Like `mfree' but don't call a mfree_hook if there is one.  */

/* Return memory to the heap.  */
void mfree(struct mdesc *mdp, void *ptr)
{
  size_t frag_nb;
  size_t i;
  int it;

  if (ptr == NULL)
    return;

  size_t block = BLOCK(ptr);

  if ((char *) ptr < (char *) mdp->heapbase || block > mdp->heapsize) {
    fprintf(stderr,"Ouch, this pointer is not mine, I refuse to free it. Give me valid pointers, or give me death!!\n");
    abort();
  }

  int type = mdp->heapinfo[block].type;

  switch (type) {
  case MMALLOC_TYPE_HEAPINFO:
    UNLOCK(mdp);
    THROWF(0, "Asked to free a fragment in a heapinfo block. I'm confused.\n");
    break;

  case MMALLOC_TYPE_FREE: /* Already free */
    UNLOCK(mdp);
    THROWF(0, "Asked to free a fragment in a block that is already free. I'm puzzled.\n");
    break;

  case MMALLOC_TYPE_UNFRAGMENTED:
    /* Get as many statistics as early as we can.  */
    mdp -> heapstats.chunks_used--;
    mdp -> heapstats.bytes_used -=
      mdp -> heapinfo[block].busy_block.size * BLOCKSIZE;
    mdp -> heapstats.bytes_free +=
      mdp -> heapinfo[block].busy_block.size * BLOCKSIZE;

    if (MC_is_active() && mdp->heapinfo[block].busy_block.ignore > 0)
      MC_unignore_heap(ptr, mdp->heapinfo[block].busy_block.busy_size);

    /* Find the free cluster previous to this one in the free list.
       Start searching at the last block referenced; this may benefit
       programs with locality of allocation.  */
    i = mdp->heapindex;
    if (i > block) {
      while (i > block) {
        i = mdp->heapinfo[i].free_block.prev;
      }
    } else {
      do {
        i = mdp->heapinfo[i].free_block.next;
      }
      while ((i != 0) && (i < block));
      i = mdp->heapinfo[i].free_block.prev;
    }

    /* Determine how to link this block into the free list.  */
    if (block == i + mdp->heapinfo[i].free_block.size) {

      /* Coalesce this block with its predecessor.  */
      mdp->heapinfo[i].free_block.size += mdp->heapinfo[block].busy_block.size;
      /* Mark all my ex-blocks as free */
      for (it=0; it<mdp->heapinfo[block].busy_block.size; it++) {
        if (mdp->heapinfo[block+it].type < 0) {
          fprintf(stderr,"Internal Error: Asked to free a block already marked as free (block=%lu it=%d type=%lu). Please report this bug.\n",
                  (unsigned long)block,it,(unsigned long)mdp->heapinfo[block].type);
          abort();
        }
        mdp->heapinfo[block+it].type = MMALLOC_TYPE_FREE;
      }

      block = i;
    } else {
      /* Really link this block back into the free list.  */
      mdp->heapinfo[block].free_block.size = mdp->heapinfo[block].busy_block.size;
      mdp->heapinfo[block].free_block.next = mdp->heapinfo[i].free_block.next;
      mdp->heapinfo[block].free_block.prev = i;
      mdp->heapinfo[i].free_block.next = block;
      mdp->heapinfo[mdp->heapinfo[block].free_block.next].free_block.prev = block;
      mdp -> heapstats.chunks_free++;
      /* Mark all my ex-blocks as free */
      for (it=0; it<mdp->heapinfo[block].free_block.size; it++) {
        if (mdp->heapinfo[block+it].type <0) {
          fprintf(stderr,"Internal error: Asked to free a block already marked as free (block=%lu it=%d/%lu type=%lu). Please report this bug.\n",
                  (unsigned long)block,it,(unsigned long)mdp->heapinfo[block].free_block.size,(unsigned long)mdp->heapinfo[block].type);
          abort();
        }
        mdp->heapinfo[block+it].type = MMALLOC_TYPE_FREE;
      }
    }

    /* Now that the block is linked in, see if we can coalesce it
       with its successor (by deleting its successor from the list
       and adding in its size).  */
    if (block + mdp->heapinfo[block].free_block.size ==
        mdp->heapinfo[block].free_block.next) {
      mdp->heapinfo[block].free_block.size
        += mdp->heapinfo[mdp->heapinfo[block].free_block.next].free_block.size;
      mdp->heapinfo[block].free_block.next
        = mdp->heapinfo[mdp->heapinfo[block].free_block.next].free_block.next;
      mdp->heapinfo[mdp->heapinfo[block].free_block.next].free_block.prev = block;
      mdp -> heapstats.chunks_free--;
    }

    /* Now see if we can return stuff to the system.  */
#if 0
          blocks = mdp -> heapinfo[block].free.size;
          if (blocks >= FINAL_FREE_BLOCKS && block + blocks == mdp -> heaplimit
          && mdp -> morecore (mdp, 0) == ADDRESS (block + blocks))
          {
          register size_t bytes = blocks * BLOCKSIZE;
          mdp -> heaplimit -= blocks;
          mdp -> morecore (mdp, -bytes);
          mdp -> heapinfo[mdp -> heapinfo[block].free.prev].free.next
          = mdp -> heapinfo[block].free.next;
          mdp -> heapinfo[mdp -> heapinfo[block].free.next].free.prev
          = mdp -> heapinfo[block].free.prev;
          block = mdp -> heapinfo[block].free.prev;
          mdp -> heapstats.chunks_free--;
          mdp -> heapstats.bytes_free -= bytes;
          }
#endif

    /* Set the next search to begin at this block.
       This is probably important to the trick where realloc returns the block to
       the system before reasking for the same block with a bigger size.  */
    mdp->heapindex = block;
    break;

  default:
    if (type < 0) {
      fprintf(stderr, "Unkown mmalloc block type.\n");
      abort();
    }

    /* Do some of the statistics.  */
    mdp -> heapstats.chunks_used--;
    mdp -> heapstats.bytes_used -= 1 << type;
    mdp -> heapstats.chunks_free++;
    mdp -> heapstats.bytes_free += 1 << type;

    frag_nb = RESIDUAL(ptr, BLOCKSIZE) >> type;

    if( mdp->heapinfo[block].busy_frag.frag_size[frag_nb] == -1){
      UNLOCK(mdp);
      THROWF(0, "Asked to free a fragment that is already free. I'm puzzled\n");
    }

    if (MC_is_active() && mdp->heapinfo[block].busy_frag.ignore[frag_nb] > 0)
      MC_unignore_heap(ptr, mdp->heapinfo[block].busy_frag.frag_size[frag_nb]);

    /* Set size used in the fragment to -1 */
    mdp->heapinfo[block].busy_frag.frag_size[frag_nb] = -1;
    mdp->heapinfo[block].busy_frag.ignore[frag_nb] = 0;

    if (mdp->heapinfo[block].busy_frag.nfree ==
        (BLOCKSIZE >> type) - 1) {
      /* If all fragments of this block are free, remove this block from its swag and free the whole block.  */
      xbt_swag_remove(&mdp->heapinfo[block],&mdp->fraghead[type]);

      /* pretend that this block is used and free it so that it gets properly coalesced with adjacent free blocks */
      mdp->heapinfo[block].type = MMALLOC_TYPE_UNFRAGMENTED;
      mdp->heapinfo[block].busy_block.size = 1;
      mdp->heapinfo[block].busy_block.busy_size = 0;

      /* Keep the statistics accurate.  */
      mdp -> heapstats.chunks_used++;
      mdp -> heapstats.bytes_used += BLOCKSIZE;
      mdp -> heapstats.chunks_free -= BLOCKSIZE >> type;
      mdp -> heapstats.bytes_free -= BLOCKSIZE;

      mfree((void *) mdp, (void *) ADDRESS(block));
    } else if (mdp->heapinfo[block].busy_frag.nfree != 0) {
      /* If some fragments of this block are free, you know what? I'm already happy. */
      ++mdp->heapinfo[block].busy_frag.nfree;
    } else {
      /* No fragments of this block were free before the one we just released,
       * so add this block to the swag and announce that
       it is the first free fragment of this block. */
      mdp->heapinfo[block].busy_frag.nfree = 1;
      mdp->heapinfo[block].freehook.prev = NULL;
      mdp->heapinfo[block].freehook.next = NULL;

      xbt_swag_insert(&mdp->heapinfo[block],&mdp->fraghead[type]);
    }
    break;
  }
}
