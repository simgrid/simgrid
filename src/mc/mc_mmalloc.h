/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MMALLOC_H
#define SIMGRID_MC_MMALLOC_H

#include <xbt/misc.h>
#include <xbt/mmalloc.h>

/** file
 *  Support for seperate heaps.
 *
 *  The possible memory modes for the modelchecker are standard and raw.
 *  Normally the system should operate in std, for switching to raw mode
 *  you must wrap the code between MC_SET_RAW_MODE and MC_UNSET_RAW_MODE.
 */

SG_BEGIN_DECL()

/* FIXME: Horrible hack! because the mmalloc library doesn't provide yet of */
/* an API to query about the status of a heap, we simply call mmstats and */
/* because I now how does structure looks like, then I redefine it here */

/* struct mstats { */
/*   size_t bytes_total;           /\* Total size of the heap. *\/ */
/*   size_t chunks_used;           /\* Chunks allocated by the user. *\/ */
/*   size_t bytes_used;            /\* Byte total of user-allocated chunks. *\/ */
/*   size_t chunks_free;           /\* Chunks in the free list. *\/ */
/*   size_t bytes_free;            /\* Byte total of chunks in the free list. *\/ */
/* }; */

SG_END_DECL()

#endif
