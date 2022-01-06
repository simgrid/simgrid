/* Support for an sbrk-like function that uses mmap. */

/* Copyright (c) 2010-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1992, 2000 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com */

#include "src/internal_config.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <errno.h>

#include "mmprivate.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define PAGE_ALIGN(addr) (void*) (((long)(addr) + xbt_pagesize - 1) &   \
                                  ~((long)xbt_pagesize - 1))

/** @brief Add memory to this heap
 *
 *  Get core for the memory region specified by MDP, using SIZE as the
 *  amount to either add to or subtract from the existing region.  Works
 *  like sbrk(), but using mmap().
 *
 *  It never returns NULL. Instead, it dies verbosely on errors.
 *
 *  @param mdp  The heap
 *  @param size Bytes to allocate for this heap (or <0 to free memory from this heap)
 */
void *mmorecore(struct mdesc *mdp, ssize_t size)
{
  void* result;                 // please keep it uninitialized to track issues
  size_t mapbytes;              /* Number of bytes to map */
  void* moveto;                 /* Address where we wish to move "break value" to */
  void* mapto;                  /* Address we actually mapped to */

  if (size == 0) {
    /* Just return the current "break" value. */
    return mdp->breakval;
  }

  if (size < 0) {
    /* We are deallocating memory.  If the amount requested would cause us to try to deallocate back past the base of
     * the mmap'd region then die verbosely.  Otherwise, deallocate the memory and return the old break value. */
    if (((char*)mdp->breakval) + size >= (char*)mdp->base) {
      result        = mdp->breakval;
      mdp->breakval = (char*)mdp->breakval + size;
      moveto = PAGE_ALIGN(mdp->breakval);
      munmap(moveto, (size_t)(((char*)mdp->top) - ((char*)moveto)) - 1);
      mdp->top = moveto;
    } else {
      fprintf(stderr,"Internal error: mmap was asked to deallocate more memory than it previously allocated. Bailling out now!\n");
      abort();
    }
  } else if ((char*)mdp->breakval + size > (char*)mdp->top) {
    /* The request would move us past the end of the currently mapped memory, so map in enough more memory to satisfy
       the request.  This means we also have to grow the mapped-to file by an appropriate amount, since mmap cannot
       be used to extend a file. */
    moveto   = PAGE_ALIGN((char*)mdp->breakval + size);
    mapbytes = (char*)moveto - (char*)mdp->top;

    /* Let's call mmap. Note that it is possible that mdp->top is 0. In this case mmap will choose the address for us.
       This call might very well overwrite an already existing memory mapping (leading to weird bugs).
    */
    mapto = mmap(mdp->top, mapbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    if (mapto == MAP_FAILED) {
      char buff[1024];
      fprintf(stderr, "Internal error: mmap returned MAP_FAILED! error: %s\n", strerror(errno));
      snprintf(buff, 1024, "cat /proc/%d/maps", getpid());
      int status = system(buff);
      if (status == -1 || !(WIFEXITED(status) && WEXITSTATUS(status) == 0))
        fprintf(stderr, "Something went wrong when trying to %s\n", buff);
      sleep(1);
      abort();
    }

    if (mdp->top == 0)
      mdp->base = mdp->breakval = mapto;

    mdp->top      = PAGE_ALIGN((char*)mdp->breakval + size);
    result        = mdp->breakval;
    mdp->breakval = (char*)mdp->breakval + size;
  } else {
    /* Memory is already mapped, we only need to increase the breakval: */
    result        = mdp->breakval;
    mdp->breakval = (char*)mdp->breakval + size;
  }
  return result;
}
