/* Allocate memory on a page boundary.
   Copyright (C) 1991 Free Software Foundation, Inc.

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

#include "mmprivate.h"
#include <unistd.h>

/* Cache the pagesize for the current host machine.  Note that if the host
   does not readily provide a getpagesize() function, we need to emulate it
   elsewhere, not clutter up this file with lots of kluges to try to figure
   it out. */

static size_t cache_pagesize;
#if NEED_DECLARATION_GETPAGESIZE
extern int getpagesize PARAMS ((void));
#endif

void*
mvalloc (void *md, size_t size)
{
  if (cache_pagesize == 0)
    {
      cache_pagesize = getpagesize ();
    }

  return (mmemalign (md, cache_pagesize, size));
}

/* Useless prototype to make gcc happy */
void* valloc (size_t size);

void* valloc (size_t size) {
  return mvalloc (NULL, size);
}
