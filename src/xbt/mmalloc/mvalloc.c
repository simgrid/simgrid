/* Allocate memory on a page boundary.
   Copyright (C) 1991 Free Software Foundation, Inc. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "mmprivate.h"
#include <unistd.h>

/* Cache the pagesize for the current host machine.  Note that if the host
   does not readily provide a getpagesize() function, we need to emulate it
   elsewhere, not clutter up this file with lots of kluges to try to figure
   it out. */

static size_t cache_pagesize;
#if NEED_DECLARATION_GETPAGESIZE
extern int getpagesize PARAMS((void));
#endif

void *mvalloc(xbt_mheap_t mdp, size_t size)
{
  if (cache_pagesize == 0) {
    cache_pagesize = getpagesize();
  }

  return (mmemalign(mdp, cache_pagesize, size));
}

