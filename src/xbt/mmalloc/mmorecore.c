/* Support for an sbrk-like function that uses mmap.
   Copyright 1992, 2000 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>             /* Prototypes for lseek */
#endif
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "mmprivate.h"

/* Cache the pagesize for the current host machine.  Note that if the host
   does not readily provide a getpagesize() function, we need to emulate it
   elsewhere, not clutter up this file with lots of kluges to try to figure
   it out. */

static size_t pagesize;

#define PAGE_ALIGN(addr) (void*) (((long)(addr) + pagesize - 1) & \
                                  ~(pagesize - 1))

/* Return MAP_PRIVATE if MDP represents /dev/zero.  Otherwise, return
   MAP_SHARED.  */
#define MAP_PRIVATE_OR_SHARED(MDP) (( MDP -> flags & MMALLOC_ANONYMOUS) \
                                    ? MAP_PRIVATE                       \
                                    : MAP_SHARED)

/* Return MAP_ANONYMOUS if MDP uses anonymous mapping. Otherwise, return 0 */
#define MAP_IS_ANONYMOUS(MDP) (((MDP) -> flags & MMALLOC_ANONYMOUS) \
                               ? MAP_ANONYMOUS                      \
                               : 0)

/* Return -1 if MDP uses anonymous mapping. Otherwise, return MDP->FD */
#define MAP_ANON_OR_FD(MDP) (((MDP) -> flags & MMALLOC_ANONYMOUS) \
                             ? -1                                 \
                             : (MDP) -> fd)

/*  Get core for the memory region specified by MDP, using SIZE as the
    amount to either add to or subtract from the existing region.  Works
    like sbrk(), but using mmap().

    It never returns NULL. Instead, it dies verbosely on errors. */

void *mmorecore(struct mdesc *mdp, int size)
{
  ssize_t test = 0;
  void *result; // please keep it uninitialized to track issues
  off_t foffset;                /* File offset at which new mapping will start */
  size_t mapbytes;              /* Number of bytes to map */
  void *moveto;                 /* Address where we wish to move "break value" to */
  void *mapto;                  /* Address we actually mapped to */
  char buf = 0;                 /* Single byte to write to extend mapped file */

  if (pagesize == 0)
    pagesize = getpagesize();

  if (size == 0) {
    /* Just return the current "break" value. */
    result = mdp->breakval;

  } else if (size < 0) {
    /* We are deallocating memory.  If the amount requested would cause
       us to try to deallocate back past the base of the mmap'd region
       then die verbosely.  Otherwise, deallocate the memory and return
       the old break value. */
    if (((char *) mdp->breakval) + size >= (char *) mdp->base) {
      result = (void *) mdp->breakval;
      mdp->breakval = (char *) mdp->breakval + size;
      moveto = PAGE_ALIGN(mdp->breakval);
      munmap(moveto,
             (size_t) (((char *) mdp->top) - ((char *) moveto)) - 1);
      mdp->top = moveto;
    } else {
      fprintf(stderr,"Internal error: mmap was asked to deallocate more memory than it previously allocated. Bailling out now!\n");
      abort();
    }
  } else {
    /* We are allocating memory. Make sure we have an open file
       descriptor if not working with anonymous memory. */
    if (!(mdp->flags & MMALLOC_ANONYMOUS) && mdp->fd < 0) {
      fprintf(stderr,"Internal error: mmap file descriptor <0 (%d), without MMALLOC_ANONYMOUS being in the flags.\n",mdp->fd);
      abort();
    } else if ((char *) mdp->breakval + size > (char *) mdp->top) {
      /* The request would move us past the end of the currently
         mapped memory, so map in enough more memory to satisfy
         the request.  This means we also have to grow the mapped-to
         file by an appropriate amount, since mmap cannot be used
         to extend a file. */
      moveto = PAGE_ALIGN((char *) mdp->breakval + size);
      mapbytes = (char *) moveto - (char *) mdp->top;
      foffset = (char *) mdp->top - (char *) mdp->base;

      if (mdp->fd > 0) {
        /* FIXME:  Test results of lseek() */
        lseek(mdp->fd, foffset + mapbytes - 1, SEEK_SET);
        test = write(mdp->fd, &buf, 1);
        if (test == -1) {
          fprintf(stderr,"Internal error: write to mmap'ed fd failed! error: %s", strerror(errno));
          abort();
        }
      }

      /* Let's call mmap. Note that it is possible that mdp->top
         is 0. In this case mmap will choose the address for us */
      mapto = mmap(mdp->top, mapbytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE_OR_SHARED(mdp) | MAP_IS_ANONYMOUS(mdp) |
                   MAP_FIXED, MAP_ANON_OR_FD(mdp), foffset);

      if (mapto == (void *) -1/* That's MAP_FAILED */) {
        fprintf(stderr,"Internal error: mmap returned MAP_FAILED! error: %s",strerror(errno));
        abort();
      }

      if (mdp->top == 0)
        mdp->base = mdp->breakval = mapto;

      mdp->top = PAGE_ALIGN((char *) mdp->breakval + size);
      result = (void *) mdp->breakval;
      mdp->breakval = (char *) mdp->breakval + size;
    } else {
      result = (void *) mdp->breakval;
      mdp->breakval = (char *) mdp->breakval + size;
    }
  }
  return (result);
}

void *__mmalloc_remap_core(xbt_mheap_t mdp)
{
  /* FIXME:  Quick hack, needs error checking and other attention. */

  return mmap(mdp->base, (char*) mdp->top - (char*) mdp->base,
              PROT_READ | PROT_WRITE | PROT_EXEC,
              MAP_PRIVATE_OR_SHARED(mdp) | MAP_FIXED, mdp->fd, 0);
}

