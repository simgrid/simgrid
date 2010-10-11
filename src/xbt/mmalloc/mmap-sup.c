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

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#include "mmprivate.h"

/* Cache the pagesize for the current host machine.  Note that if the host
   does not readily provide a getpagesize() function, we need to emulate it
   elsewhere, not clutter up this file with lots of kluges to try to figure
   it out. */

static size_t pagesize;
#if NEED_DECLARATION_GETPAGESIZE
extern int getpagesize(void);
#endif

#define PAGE_ALIGN(addr) (void*) (((long)(addr) + pagesize - 1) & \
				    ~(pagesize - 1))

/* Return MAP_PRIVATE if MDP represents /dev/zero.  Otherwise, return
   MAP_SHARED.  */

#define MAP_PRIVATE_OR_SHARED(MDP) (( MDP -> flags & MMALLOC_ANONYMOUS) \
                                    ? MAP_PRIVATE \
                                    : MAP_SHARED)

/* Return MAP_ANONYMOUS if MDP uses anonymous mapping. Otherwise, return 0 */

#define MAP_IS_ANONYMOUS(MDP) (((MDP) -> flags & MMALLOC_ANONYMOUS) \
                              ? MAP_ANONYMOUS \
                              : 0)

/* Return -1 if MDP uses anonymous mapping. Otherwise, return MDP->FD */
#define MAP_ANON_OR_FD(MDP) (((MDP) -> flags & MMALLOC_ANONYMOUS) \
                              ? -1 \
                  			      : (MDP) -> fd)

/*  Get core for the memory region specified by MDP, using SIZE as the
    amount to either add to or subtract from the existing region.  Works
    like sbrk(), but using mmap(). */

void *__mmalloc_mmap_morecore(struct mdesc *mdp, int size)
{
  ssize_t test = 0;
  void *result = NULL;
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
       then do nothing, and return NULL.  Otherwise, deallocate the
       memory and return the old break value. */
    if (((char *) mdp->breakval) + size >= (char *) mdp->base) {
      result = (void *) mdp->breakval;
      mdp->breakval = (char *) mdp->breakval + size;
      moveto = PAGE_ALIGN(mdp->breakval);
      munmap(moveto,
             (size_t) (((char *) mdp->top) - ((char *) moveto)) - 1);
      mdp->top = moveto;
    }
  } else {
    /* We are allocating memory. Make sure we have an open file
       descriptor if not working with anonymous memory. */
    if (!(mdp->flags & MMALLOC_ANONYMOUS) && mdp->fd < 0) {
      result = NULL;
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
        /* FIXME:  Test results of lseek() and write() */
        lseek(mdp->fd, foffset + mapbytes - 1, SEEK_SET);
        test = write(mdp->fd, &buf, 1);
      }

      /* Let's call mmap. Note that it is possible that mdp->top
         is 0. In this case mmap will choose the address for us */
      mapto = mmap(mdp->top, mapbytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE_OR_SHARED(mdp) | MAP_IS_ANONYMOUS(mdp) |
                   MAP_FIXED, MAP_ANON_OR_FD(mdp), foffset);

      if (mapto != (void *) -1) {

        if (mdp->top == 0)
          mdp->base = mdp->breakval = mapto;

        mdp->top = PAGE_ALIGN((char *) mdp->breakval + size);
        result = (void *) mdp->breakval;
        mdp->breakval = (char *) mdp->breakval + size;
      }
    } else {
      result = (void *) mdp->breakval;
      mdp->breakval = (char *) mdp->breakval + size;
    }
  }
  return (result);
}

void *__mmalloc_remap_core(struct mdesc *mdp)
{
  void *base;

  /* FIXME:  Quick hack, needs error checking and other attention. */

  base = mmap(mdp->base, (char *) mdp->top - (char *) mdp->base,
              PROT_READ | PROT_WRITE | PROT_EXEC,
              MAP_PRIVATE_OR_SHARED(mdp) | MAP_FIXED, mdp->fd, 0);
  return ((void *) base);
}

void *mmalloc_findbase(int size)
{
  int fd;
  int flags;
  void *base = NULL;

#ifdef MAP_ANONYMOUS
  flags = MAP_PRIVATE | MAP_ANONYMOUS;
  fd = -1;
#else
#ifdef MAP_FILE
  flags = MAP_PRIVATE | MAP_FILE;
#else
  flags = MAP_PRIVATE;
#endif
  fd = open("/dev/zero", O_RDWR);
  if (fd != -1) {
    return ((void *) NULL);
  }
#endif
  base = mmap(0, size, PROT_READ | PROT_WRITE, flags, fd, 0);
  if (base != (void *) -1) {
    munmap(base, (size_t) size);
  }
  if (fd != -1) {
    close(fd);
  }
  if (base == 0) {
    /* Don't allow mapping at address zero.  We use that value
       to signal an error return, and besides, it is useful to
       catch NULL pointers if it is unmapped.  Instead start
       at the next page boundary. */
    base = (void *) (long) getpagesize();
  } else if (base == (void *) -1) {
    base = NULL;
  }
  return ((void *) base);
}
