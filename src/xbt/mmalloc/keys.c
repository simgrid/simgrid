/* Access for application keys in mmap'd malloc managed region.
   Copyright 1992 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com
   This file was then part of the GNU C Library. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */




/* This module provides access to some keys that the application can use to
   provide persistent access to locations in the mapped memory section.
   The intent is that these keys are to be used sparingly as sort of
   persistent global variables which the application can use to reinitialize
   access to data in the mapped region.

   For the moment, these keys are simply stored in the malloc descriptor
   itself, in an array of fixed length.  This should be fixed so that there
   can be an unlimited number of keys, possibly using a multilevel access
   scheme of some sort. */

#include "mmprivate.h"

int mmalloc_setkey(void *md, int keynum, void *key)
{
  struct mdesc *mdp = (struct mdesc *) md;
  int result = 0;

  LOCK(mdp);
  if ((mdp != NULL) && (keynum >= 0) && (keynum < MMALLOC_KEYS)) {
    mdp->keys[keynum] = key;
    result++;
  }
  UNLOCK(mdp);
  return (result);
}

void *mmalloc_getkey(void *md, int keynum)
{
  struct mdesc *mdp = (struct mdesc *) md;
  void *keyval = NULL;

  if ((mdp != NULL) && (keynum >= 0) && (keynum < MMALLOC_KEYS)) {
    keyval = mdp->keys[keynum];
  }
  return (keyval);
}
