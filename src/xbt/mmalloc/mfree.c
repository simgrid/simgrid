/* Free a block of memory allocated by `mmalloc'.
   Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish.  (fnf@cygnus.com) */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmprivate.h"
#include "xbt/ex.h"

/* Return memory to the heap.
   Like `mfree' but don't call a mfree_hook if there is one.  */

/* Return memory to the heap.  */
void mfree(struct mdesc *mdp, void *ptr)
{
  int type;
  size_t block, frag_nb;
  register size_t i;
  struct list *prev, *next;
  int it;

  if (ptr == NULL)
    return;

  block = BLOCK(ptr);

  if ((char *) ptr < (char *) mdp->heapbase || block > mdp->heapsize) {
    fprintf(stderr,"Ouch, this pointer is not mine. I refuse to free it. I refuse it to death!!\n");
    abort();
  }


  type = mdp->heapinfo[block].type;

  switch (type) {
  case -1: /* Already free */
    THROWF(system_error, 0, "Asked to free a fragment in a block that is already free. I'm puzzled\n");
    break;
    
  case 0:
    /* Get as many statistics as early as we can.  */
    mdp -> heapstats.chunks_used--;
    mdp -> heapstats.bytes_used -=
      mdp -> heapinfo[block].busy_block.size * BLOCKSIZE;
    mdp -> heapstats.bytes_free +=
      mdp -> heapinfo[block].busy_block.size * BLOCKSIZE;

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
        if (mdp->heapinfo[block+it].type <0) {
          fprintf(stderr,"Internal Error: Asked to free a block already marked as free (block=%lu it=%d type=%lu). Please report this bug.\n",
                  (unsigned long)block,it,(unsigned long)mdp->heapinfo[block].type);
          abort();
        }
        mdp->heapinfo[block+it].type = -1;
      }

      block = i;
    } else {
      //fprintf(stderr,"Free block %d to %d (as a new chunck)\n",block,block+mdp->heapinfo[block].busy_block.size);
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
        mdp->heapinfo[block+it].type = -1;
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
    /*    blocks = mdp -> heapinfo[block].free.size;
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
          } */

    /* Set the next search to begin at this block.  */
    mdp->heapindex = block;
    break;

  default:
    /* Do some of the statistics.  */
    mdp -> heapstats.chunks_used--;
    mdp -> heapstats.bytes_used -= 1 << type;
    mdp -> heapstats.chunks_free++;
    mdp -> heapstats.bytes_free += 1 << type;

    /* Get the address of the first free fragment in this block.  */
    prev = (struct list *)
      ((char *) ADDRESS(block) +
       (mdp->heapinfo[block].busy_frag.first << type));

    /* Set size used in the fragment to 0 */
    frag_nb = RESIDUAL(ptr, BLOCKSIZE) >> type;

    if( mdp->heapinfo[block].busy_frag.frag_size[frag_nb] == 0)
      THROWF(system_error, 0, "Asked to free a fragment that is already free. I'm puzzled\n");

    mdp->heapinfo[block].busy_frag.frag_size[frag_nb] = 0;

    if (mdp->heapinfo[block].busy_frag.nfree ==
        (BLOCKSIZE >> type) - 1) {
      /* If all fragments of this block are free, remove them
         from the fragment list and free the whole block.  */
      next = prev;
      for (i = 1; i < (size_t) (BLOCKSIZE >> type); ++i) {
        next = next->next;
      }
      prev->prev->next = next;
      if (next != NULL) {
        next->prev = prev->prev;
      }
      /* pretend that this block is used and free it so that it gets properly coalesced with adjacent free blocks */
      mdp->heapinfo[block].type = 0;
      mdp->heapinfo[block].busy_block.size = 1;
      mdp->heapinfo[block].busy_block.busy_size = 0;
      
      /* Keep the statistics accurate.  */
      mdp -> heapstats.chunks_used++;
      mdp -> heapstats.bytes_used += BLOCKSIZE;
      mdp -> heapstats.chunks_free -= BLOCKSIZE >> type;
      mdp -> heapstats.bytes_free -= BLOCKSIZE;
      
      mfree((void *) mdp, (void *) ADDRESS(block));
    } else if (mdp->heapinfo[block].busy_frag.nfree != 0) {
      /* If some fragments of this block are free, link this
         fragment into the fragment list after the first free
         fragment of this block. */
      next = (struct list *) ptr;
      next->next = prev->next;
      next->prev = prev;
      prev->next = next;
      if (next->next != NULL) {
        next->next->prev = next;
      }
      ++mdp->heapinfo[block].busy_frag.nfree;
    } else {
      /* No fragments of this block were free before the one we just released,
       * so link this fragment into the fragment list and announce that
       it is the first free fragment of this block. */
      prev = (struct list *) ptr;
      mdp->heapinfo[block].busy_frag.nfree = 1;
      mdp->heapinfo[block].busy_frag.first = frag_nb;
      prev->next = mdp->fraghead[type].next;
      prev->prev = &mdp->fraghead[type];
      prev->prev->next = prev;
      if (prev->next != NULL) {
        prev->next->prev = prev;
      }
    }
    break;
  }
}
