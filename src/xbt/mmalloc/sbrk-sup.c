/* Support for sbrk() regions.
   Copyright 1992, 2000 Free Software Foundation, Inc.
   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>             /* Prototypes for sbrk (maybe) */

#include <string.h>             /* Prototypes for memcpy, memmove, memset, etc */

#include "xbt.h"
#include "mmprivate.h"

static void *sbrk_morecore(struct mdesc *mdp, int size);
#if NEED_DECLARATION_SBRK
extern void *sbrk(int size);
#endif


/* Use sbrk() to get more core. */

static void *sbrk_morecore(mdp, size)
struct mdesc *mdp;
int size;
{
  void *result;

  if ((result = sbrk(size)) == (void *) -1) {
    result = NULL;
  } else {
    mdp->breakval = (char *) mdp->breakval + size;
    mdp->top = (char *) mdp->top + size;
  }
  return (result);
}
