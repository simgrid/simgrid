/* Change the size of a block allocated by `mmalloc'. */

/* Copyright (c) 2010-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1990, 1991 Free Software Foundation
   Written May 1989 by Mike Haertel. */

#include <string.h>             /* Prototypes for memcpy, memmove, memset, etc */
#include <stdlib.h> /* abort */

#include "mmprivate.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Resize the given region to the new size, returning a pointer to the (possibly moved) region. This is optimized for
 * speed; some benchmarks seem to indicate that greater compactness is achieved by unconditionally allocating and
 * copying to a new region.  This module has incestuous knowledge of the internals of both mfree and mmalloc. */

void *mrealloc(xbt_mheap_t mdp, void *ptr, size_t size)
{
  void *result;
  size_t blocks;

  /* Only keep real realloc, and reroute hidden malloc and free to the relevant functions */
  if (size == 0) {
    mfree(mdp, ptr);
    return mmalloc(mdp, 0);
  } else if (ptr == NULL) {
    return mmalloc(mdp, size);
  }

  if ((char *) ptr < (char *) mdp->heapbase || BLOCK(ptr) > mdp->heapsize) {
    printf("FIXME. Ouch, this pointer is not mine, refusing to proceed (another solution would be to malloc "
           "it instead of reallocing it, see source code)\n");
    abort();
  }

  size_t requested_size = size; // The amount of memory requested by user, for real

  /* Work even if the user was stupid enough to ask a ridiculously small block (even 0-length),
   *    ie return a valid block that can be realloced and freed.
   * glibc malloc does not use this trick but return a constant pointer, but we need to enlist the free fragments later on.
   */
  if (size < SMALLEST_POSSIBLE_MALLOC)
    size = SMALLEST_POSSIBLE_MALLOC;

  size_t block = BLOCK(ptr);

  int type = mdp->heapinfo[block].type;

  switch (type) {
  case MMALLOC_TYPE_HEAPINFO:
    fprintf(stderr, "Asked realloc a fragment coming from a heapinfo block. I'm confused.\n");
    abort();
    break;

  case MMALLOC_TYPE_FREE:
    fprintf(stderr, "Asked realloc a fragment coming from a *free* block. I'm puzzled.\n");
    abort();
    break;

  case MMALLOC_TYPE_UNFRAGMENTED:
    /* Maybe reallocate a large block to a small fragment.  */

    if (size <= BLOCKSIZE / 2) { // Full block -> Fragment; no need to optimize for time

      result = mmalloc(mdp, size);
      memcpy(result, ptr, requested_size);
      mfree(mdp, ptr);
      return (result);
    }

    /* Full blocks -> Full blocks; see if we can hold it in place. */
    blocks = BLOCKIFY(size);
    if (blocks < mdp->heapinfo[block].busy_block.size) {
      int it;
      /* The new size is smaller; return excess memory to the free list. */
      for (it= block+blocks; it< mdp->heapinfo[block].busy_block.size ; it++){
        mdp->heapinfo[it].type = MMALLOC_TYPE_UNFRAGMENTED; // FIXME that should be useless, type should already be 0 here
        mdp->heapinfo[it].busy_block.ignore = 0;
        mdp->heapinfo[it].busy_block.size = 0;
        mdp->heapinfo[it].busy_block.busy_size = 0;
      }

      mdp->heapinfo[block + blocks].busy_block.size = mdp->heapinfo[block].busy_block.size - blocks;
      mfree(mdp, ADDRESS(block + blocks));

      mdp->heapinfo[block].busy_block.size = blocks;
      mdp->heapinfo[block].busy_block.busy_size = requested_size;
      mdp->heapinfo[block].busy_block.ignore = 0;

      result = ptr;
    } else if (blocks == mdp->heapinfo[block].busy_block.size) {

      /* No block size change necessary; only update the requested size  */
      result = ptr;
      mdp->heapinfo[block].busy_block.busy_size = requested_size;
      mdp->heapinfo[block].busy_block.ignore = 0;

    } else {
      /* Won't fit, so allocate a new region that will.
         Free the old region first in case there is sufficient adjacent free space to grow without moving.
         This trick mandates using a specific version of mmalloc that does not memset the memory to 0 after
           action for obvious reasons. */
      blocks = mdp->heapinfo[block].busy_block.size;
      /* Prevent free from actually returning memory to the system.  */
      size_t oldlimit = mdp->heaplimit;
      mdp->heaplimit = 0;
      mfree(mdp, ptr);
      mdp->heaplimit = oldlimit;

      result = mmalloc_no_memset(mdp, requested_size);

      if (ptr != result)
        memmove(result, ptr, blocks * BLOCKSIZE);
      /* FIXME: we should memset the end of the recently area */
    }
    break;

  default: /* Fragment -> ??; type=logarithm to base two of the fragment size.  */

    if (type < 0) {
      fprintf(stderr, "Unkown mmalloc block type.\n");
      abort();
    }

    if (size > (size_t) (1 << (type - 1)) && size <= (size_t) (1 << type)) {
      /* The new size is the same kind of fragment.  */

      result = ptr;
      int frag_nb = RESIDUAL(result, BLOCKSIZE) >> type;
      mdp->heapinfo[block].busy_frag.frag_size[frag_nb] = requested_size;
      mdp->heapinfo[block].busy_frag.ignore[frag_nb] = 0;

    } else { /* fragment -> Either other fragment, or block */
      /* The new size is different; allocate a new space,
         and copy the lesser of the new size and the old. */

      result = mmalloc(mdp, requested_size);

      memcpy(result, ptr, MIN(requested_size, (size_t) 1 << type));
      mfree(mdp, ptr);
    }
    break;
  }
  return (result);
}
