/* Finish access to a mmap'd malloc managed region.
   Copyright 1992 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com
   This file was then part of the GNU C Library. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>   /* close */
#include <sys/types.h>
#include "mmprivate.h"

/* Terminate access to a mmalloc managed region by unmapping all memory pages
   associated with the region, and closing the file descriptor if it is one
   that we opened.

   Returns NULL on success.

   Returns the malloc descriptor on failure, which can subsequently be used
   for further action, such as obtaining more information about the nature of
   the failure by examining the preserved errno value.

   Note that the malloc descriptor that we are using is currently located in
   region we are about to unmap, so we first make a local copy of it on the
   stack and use the copy. */

void*
mmalloc_detach (void *md)
{
  struct mdesc mtemp;

  if (md != NULL)
    {

      mtemp = *(struct mdesc *) md;
      xbt_os_mutex_destroy(((struct mdesc*)md)->mutex);
      
      /* Now unmap all the pages associated with this region by asking for a
	 negative increment equal to the current size of the region. */
      
      if ((mtemp.morecore (&mtemp, (char*)mtemp.base - (char*)mtemp.breakval)) == NULL)
	{
	  /* Deallocating failed.  Update the original malloc descriptor
	     with any changes */
	  *(struct mdesc *) md = mtemp;
	}
      else
	{
	  if (mtemp.flags & MMALLOC_DEVZERO)
	    {
	      close (mtemp.fd);
	    }
	  md = NULL;
	}
    }

  return (md);
}
