/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
   This file was then part of the GNU C Library. */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmprivate.h"

void *
mmemalign (void *md, size_t alignment, size_t size)
{
  void* result;
  unsigned long int adj;
  struct alignlist *l;
  struct mdesc *mdp;

  if ((result = mmalloc (md, size + alignment - 1)) != NULL)
    {
      adj = RESIDUAL (result, alignment);
      if (adj != 0)
	{
	  mdp = MD_TO_MDP (md);
	  for (l = mdp -> aligned_blocks; l != NULL; l = l -> next)
	    {
	      if (l -> aligned == NULL)
		{
		  /* This slot is free.  Use it.  */
		  break;
		}
	    }
	  if (l == NULL)
	    {
	      l = (struct alignlist *) mmalloc (md, sizeof (struct alignlist));
	      if (l == NULL)
		{
		  mfree (md, result);
		  return (NULL);
		}
	      l -> next = mdp -> aligned_blocks;
	      mdp -> aligned_blocks = l;
	    }
	  l -> exact = result;
	  result = l -> aligned = (char*) result + alignment - adj;
	}
    }
  return (result);
}
