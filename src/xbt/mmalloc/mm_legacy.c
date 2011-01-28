/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */

#include "mmprivate.h"
#include "gras_config.h"

static void *__mmalloc_current_heap = NULL;     /* The heap we are currently using. */

#include "xbt_modinter.h"

void *mmalloc_get_current_heap(void)
{
  return __mmalloc_current_heap;
}

void mmalloc_set_current_heap(void *new_heap)
{
  __mmalloc_current_heap = new_heap;
}

#ifdef MMALLOC_WANT_OVERIDE_LEGACY
void *malloc(size_t n)
{
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  void *ret = mmalloc(mdp, n);
  UNLOCK(mdp);

  return ret;
}

void *calloc(size_t nmemb, size_t size)
{
  size_t total_size = nmemb * size;
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  void *ret = mmalloc(mdp, total_size);
  UNLOCK(mdp);

  /* Fill the allocated memory with zeroes to mimic calloc behaviour */
  memset(ret, '\0', total_size);

  return ret;
}

void *realloc(void *p, size_t s)
{
  void *ret = NULL;
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_MMAP
  if (!mdp)
    mmalloc_preinit();
#endif
  LOCK(mdp);
  if (s) {
    if (p)
      ret = mrealloc(mdp, p, s);
    else
      ret = mmalloc(mdp, s);
  } else {
    mfree(mdp, p);
  }
  UNLOCK(mdp);

  return ret;
}

void free(void *p)
{
  void *mdp = __mmalloc_current_heap;
#ifdef HAVE_GTNETS
  if(!mdp) return;
#endif
  LOCK(mdp);
  mfree(mdp, p);
  UNLOCK(mdp);
}
#endif

/* Make sure it works with md==NULL */

/* Safety gap from the heap's break address.
 * Try to increase this first if you experience strange errors under
 * valgrind. */
#define HEAP_OFFSET   (128UL<<20)

void *mmalloc_get_default_md(void)
{
  xbt_assert(__mmalloc_default_mdp);
  return __mmalloc_default_mdp;
}

static void mmalloc_fork_prepare(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      LOCK(mdp);
      if(mdp->fd >= 0){
        mdp->refcount++;
      }
      mdp = mdp->next_mdesc;
    }
  }
}

static void mmalloc_fork_parent(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      if(mdp->fd < 0)
        UNLOCK(mdp);
      mdp = mdp->next_mdesc;
    }
  }
}

static void mmalloc_fork_child(void)
{
  struct mdesc* mdp = NULL;
  if ((mdp =__mmalloc_default_mdp)){
    while(mdp){
      UNLOCK(mdp);
      mdp = mdp->next_mdesc;
    }
  }
}

/* Initialize the default malloc descriptor. */
void mmalloc_preinit(void)
{
  int res;
  if (!__mmalloc_default_mdp) {
    unsigned long mask = ~((unsigned long)getpagesize() - 1);
    void *addr = (void*)(((unsigned long)sbrk(0) + HEAP_OFFSET) & mask);
    __mmalloc_default_mdp = mmalloc_attach(-1, addr);
    /* Fixme? only the default mdp in protected against forks */
    res = xbt_os_thread_atfork(mmalloc_fork_prepare,
			       mmalloc_fork_parent, mmalloc_fork_child);
    if (res != 0)
      THROWF(system_error,0,"xbt_os_thread_atfork() failed: return value %d",res);
  }
  xbt_assert(__mmalloc_default_mdp != NULL);
}

void mmalloc_postexit(void)
{
  /* Do not detach the default mdp or ldl won't be able to free the memory it allocated since we're in memory */
  //  mmalloc_detach(__mmalloc_default_mdp);
  mmalloc_pre_detach(__mmalloc_default_mdp);
}
