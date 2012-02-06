/* Change the size of a block allocated by `mmalloc'.
   Copyright 1990, 1991 Free Software Foundation
		  Written May 1989 by Mike Haertel. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string.h>             /* Prototypes for memcpy, memmove, memset, etc */
#include <stdlib.h> /* abort */

#include "mmprivate.h"

/* Resize the given region to the new size, returning a pointer
   to the (possibly moved) region.  This is optimized for speed;
   some benchmarks seem to indicate that greater compactness is
   achieved by unconditionally allocating and copying to a
   new region.  This module has incestuous knowledge of the
   internals of both mfree and mmalloc. */

void *mrealloc(xbt_mheap_t mdp, void *ptr, size_t size)
{
  void *result;
  int type;
  size_t block, blocks, oldlimit;

  /* Only keep real realloc, and reroute hidden malloc and free to the relevant functions */
  if (size == 0) {
    mfree(mdp, ptr);
    return mmalloc(mdp, 0);
  } else if (ptr == NULL) {
    return mmalloc(mdp, size);
  }

  //printf("(%s)realloc %p to %d...",xbt_thread_self_name(),ptr,(int)size);

  if ((char *) ptr < (char *) mdp->heapbase || BLOCK(ptr) > mdp->heapsize) {
    printf
        ("FIXME. Ouch, this pointer is not mine, refusing to proceed (another solution would be to malloc it instead of reallocing it, see source code)\n");
    result = mmalloc(mdp, size);
    abort();
    return result;
  }

  block = BLOCK(ptr);

  type = mdp->heapinfo[block].type;

  switch (type) {
  case -1:
    fprintf(stderr, "Asked realloc a fragment comming from a *free* block. I'm puzzled.\n");
    abort();
    break;

  case 0:
    /* Maybe reallocate a large block to a small fragment.  */

    if (size <= BLOCKSIZE / 2) { // Full block -> Fragment; no need to optimize for time

      result = mmalloc(mdp, size);
      if (result != NULL) { // useless (mmalloc never returns NULL), but harmless
        memcpy(result, ptr, size);
        mfree(mdp, ptr);
        return (result);
      }
    }

    /* The new size is a large allocation as well;
       see if we can hold it in place. */
    blocks = BLOCKIFY(size);
    if (blocks < mdp->heapinfo[block].busy_block.size) {
    	int it;
      /* The new size is smaller; return excess memory to the free list. */
      //printf("(%s) return excess memory...",xbt_thread_self_name());
     for (it= block+blocks; it< mdp->heapinfo[block].busy_block.size ; it++)
    	 mdp->heapinfo[it].type = 0;
      mdp->heapinfo[block + blocks].busy_block.size
          = mdp->heapinfo[block].busy_block.size - blocks;
      mdp->heapinfo[block].busy_block.size = blocks;
      mdp->heapinfo[block].busy_block.busy_size = size;

      mfree(mdp, ADDRESS(block + blocks));
      result = ptr;
    } else if (blocks == mdp->heapinfo[block].busy_block.size) {
      /* No size change necessary.  */
      result = ptr;
    } else {
      /* Won't fit, so allocate a new region that will.
         Free the old region first in case there is sufficient
         adjacent free space to grow without moving. */
      blocks = mdp->heapinfo[block].busy_block.size;
      /* Prevent free from actually returning memory to the system.  */
      oldlimit = mdp->heaplimit;
      mdp->heaplimit = 0;
      mfree(mdp, ptr);
      mdp->heaplimit = oldlimit;
      result = mmalloc(mdp, size);
      if (result == NULL) {
        mmalloc(mdp, blocks * BLOCKSIZE);
        return (NULL);
      }
      if (ptr != result)
        memmove(result, ptr, blocks * BLOCKSIZE);
    }
    break;

  default:
    /* Old size is a fragment; type is logarithm
       to base two of the fragment size.  */
    if (size > (size_t) (1 << (type - 1)) && size <= (size_t) (1 << type)) {
      /* The new size is the same kind of fragment.  */
      //printf("(%s) new size is same kind of fragment...",xbt_thread_self_name());
      result = ptr;
    } else {
      /* The new size is different; allocate a new space,
         and copy the lesser of the new size and the old. */
      //printf("(%s) new size is different...",xbt_thread_self_name());

      result = mmalloc(mdp, size);
      if (result == NULL)
        return (NULL);

      memcpy(result, ptr, MIN(size, (size_t) 1 << type));
      mfree(mdp, ptr);
    }
    break;
  }
  //printf("(%s) Done reallocing: %p\n",xbt_thread_self_name(),result);fflush(stdout);
  return (result);
}
