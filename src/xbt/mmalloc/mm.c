/* Build the entire mmalloc library as a single object module. This
   avoids having clients pick up part of their allocation routines
   from mmalloc and part from libc, which results in undefined
   behavior.  It should also still be possible to build the library
   as a standard library with multiple objects. */

/* Copyright (c) 2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1996, 2000 Free Software Foundation  */

#define _GNU_SOURCE
#include "src/internal_config.h"
#if HAVE_UNISTD_H
#include <unistd.h>             /* Prototypes for lseek, sbrk (maybe) */
#endif

#include "mfree.c"
#include "mmalloc.c"
#include "mrealloc.c"
#include "mmorecore.c"
#include "mm_legacy.c"
#include "mm_module.c"
