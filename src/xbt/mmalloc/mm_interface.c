/* External interface to a mmap'd malloc managed region. */

/* Copyright (c) 2012-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1992, 2000 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com

   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <fcntl.h> /* After sys/types.h, at least for dpx/2.  */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mmprivate.h"

// This is the underlying implementation of mmalloc_get_bytes_used_remote.
// Is it used directly to evaluate the bytes used from a different process.
size_t mmalloc_get_bytes_used_remote(size_t heaplimit, const malloc_info* heapinfo)
{
  int bytes = 0;
  for (size_t i = 0; i < heaplimit; ++i) {
    if (heapinfo[i].type == MMALLOC_TYPE_UNFRAGMENTED) {
      if (heapinfo[i].busy_block.busy_size > 0)
        bytes += heapinfo[i].busy_block.busy_size;
    } else if (heapinfo[i].type > 0) {
      for (size_t j = 0; j < (size_t)(BLOCKSIZE >> heapinfo[i].type); j++) {
        if (heapinfo[i].busy_frag.frag_size[j] > 0)
          bytes += heapinfo[i].busy_frag.frag_size[j];
      }
    }
  }
  return bytes;
}
