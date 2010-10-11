/* Access the statistics maintained by `mmalloc'.
   Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Modified Mar 1992 by Fred Fish.  (fnf@cygnus.com) */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmprivate.h"

/* FIXME:  See the comment in mmprivate.h where struct mstats is defined.
   None of the internal mmalloc structures should be externally visible
   outside the library. */

struct mstats mmstats(void *md)
{
  struct mstats result;
  struct mdesc *mdp;

  mdp = MD_TO_MDP(md);
  result.bytes_total = (char *) mdp->top - (char *) mdp;
  result.chunks_used = mdp->heapstats.chunks_used;
  result.bytes_used = mdp->heapstats.bytes_used;
  result.chunks_free = mdp->heapstats.chunks_free;
  result.bytes_free = mdp->heapstats.bytes_free;
  return (result);
}
