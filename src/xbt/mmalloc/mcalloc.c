/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
   This file was then part of the GNU C Library. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/types.h>  /* GCC on HP/UX needs this before string.h. */
#include <string.h>	/* Prototypes for memcpy, memmove, memset, etc */

#include "mmprivate.h"

/* Allocate an array of NMEMB elements each SIZE bytes long.
   The entire array is initialized to zeros.  */

void *
mcalloc (void *md, register size_t nmemb, register size_t size)
{
  register void* result;

  if ((result = mmalloc (md, nmemb * size)) != NULL)
    {
      memset (result, 0, nmemb * size);
    }
  return (result);
}
