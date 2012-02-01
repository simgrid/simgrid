/* Free a block of memory allocated by `mmalloc'.
   Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish.  (fnf@cygnus.com) */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmprivate.h"

/* Return memory to the heap.
   Like `mfree' but don't call a mfree_hook if there is one.  */

void __mmalloc_free(struct mdesc *mdp, void *ptr)
{
  int type;
  size_t block;
  register size_t i;
  struct list *prev, *next;

  block = BLOCK(ptr);

  if ((char *) ptr < (char *) mdp->heapbase || block > mdp->heapsize) {
    printf("Ouch, this pointer is not mine. I refuse to free it.\n");
    return;
  }


  type = mdp->heapinfo[block].busy.type;
  switch (type) {
  case 0:
    /* Find the free cluster previous to this one in the free list.
       Start searching at the last block referenced; this may benefit
       programs with locality of allocation.  */
    i = mdp->heapindex;
    if (i > block) {
      while (i > block) {
        i = mdp->heapinfo[i].free.prev;
      }
    } else {
      do {
        i = mdp->heapinfo[i].free.next;
      }
      while ((i != 0) && (i < block));
      i = mdp->heapinfo[i].free.prev;
    }

    /* Determine how to link this block into the free list.  */
    if (block == i + mdp->heapinfo[i].free.size) {
      /* Coalesce this block with its predecessor.  */
      mdp->heapinfo[i].free.size += mdp->heapinfo[block].busy.info.block.size;
      block = i;
    } else {
      /* Really link this block back into the free list.  */
      mdp->heapinfo[block].free.size = mdp->heapinfo[block].busy.info.block.size;
      mdp->heapinfo[block].free.next = mdp->heapinfo[i].free.next;
      mdp->heapinfo[block].free.prev = i;
      mdp->heapinfo[i].free.next = block;
      mdp->heapinfo[mdp->heapinfo[block].free.next].free.prev = block;
    }

    /* Now that the block is linked in, see if we can coalesce it
       with its successor (by deleting its successor from the list
       and adding in its size).  */
    if (block + mdp->heapinfo[block].free.size ==
        mdp->heapinfo[block].free.next) {
      mdp->heapinfo[block].free.size
          += mdp->heapinfo[mdp->heapinfo[block].free.next].free.size;
      mdp->heapinfo[block].free.next
          = mdp->heapinfo[mdp->heapinfo[block].free.next].free.next;
      mdp->heapinfo[mdp->heapinfo[block].free.next].free.prev = block;
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
    /* Get the address of the first free fragment in this block.  */
    prev = (struct list *)
        ((char *) ADDRESS(block) +
         (mdp->heapinfo[block].busy.info.frag.first << type));

    if (mdp->heapinfo[block].busy.info.frag.nfree ==
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
      mdp->heapinfo[block].busy.type = 0;
      mdp->heapinfo[block].busy.info.block.size = 1;
      mdp->heapinfo[block].busy.info.block.busy_size = 0;

      mfree((void *) mdp, (void *) ADDRESS(block));
    } else if (mdp->heapinfo[block].busy.info.frag.nfree != 0) {
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
      ++mdp->heapinfo[block].busy.info.frag.nfree;
    } else {
      /* No fragments of this block are free, so link this
         fragment into the fragment list and announce that
         it is the first free fragment of this block. */
      prev = (struct list *) ptr;
      mdp->heapinfo[block].busy.info.frag.nfree = 1;
      mdp->heapinfo[block].busy.info.frag.first =
          RESIDUAL(ptr, BLOCKSIZE) >> type;
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

/* Return memory to the heap.  */

void mfree(xbt_mheap_t mdp, void *ptr)
{
  register struct alignlist *l;

  if (ptr != NULL) {
    for (l = mdp->aligned_blocks; l != NULL; l = l->next) {
      if (l->aligned == ptr) {
        l->aligned = NULL;      /* Mark the slot in the list as free. */
        ptr = l->exact;
        break;
      }
    }
    __mmalloc_free(mdp, ptr);
  }
}
