/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */

#include "mmprivate.h"
#include "gras_config.h"
#include <math.h>


/* The mmalloc() package can use a single implicit malloc descriptor
   for mmalloc/mrealloc/mfree operations which do not supply an explicit
   descriptor.  This allows mmalloc() to provide
   backwards compatibility with the non-mmap'd version. */
xbt_mheap_t __mmalloc_default_mdp = NULL;


static xbt_mheap_t __mmalloc_current_heap = NULL;     /* The heap we are currently using. */

xbt_mheap_t mmalloc_get_current_heap(void)
{
  return __mmalloc_current_heap;
}

void mmalloc_set_current_heap(xbt_mheap_t new_heap)
{
  __mmalloc_current_heap = new_heap;
}

#ifdef MMALLOC_WANT_OVERIDE_LEGACY
void *malloc(size_t n)
{
  xbt_mheap_t mdp = __mmalloc_current_heap ?: (xbt_mheap_t) mmalloc_preinit();

  LOCK(mdp);
  void *ret = mmalloc(mdp, n);
  UNLOCK(mdp);

  return ret;
}

void *calloc(size_t nmemb, size_t size)
{
  xbt_mheap_t mdp = __mmalloc_current_heap ?: (xbt_mheap_t) mmalloc_preinit();

  LOCK(mdp);
  void *ret = mmalloc(mdp, nmemb*size);
  UNLOCK(mdp);
  memset(ret, 0, nmemb * size);


  return ret;
}

void *realloc(void *p, size_t s)
{
  void *ret = NULL;
  xbt_mheap_t mdp = __mmalloc_current_heap ?: (xbt_mheap_t) mmalloc_preinit();

  LOCK(mdp);
  ret = mrealloc(mdp, p, s);
  UNLOCK(mdp);

  return ret;
}

void free(void *p)
{
  xbt_mheap_t mdp = __mmalloc_current_heap ?: (xbt_mheap_t) mmalloc_preinit();

  LOCK(mdp);
  mfree(mdp, p);
  UNLOCK(mdp);
}
#endif


