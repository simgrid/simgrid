/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */
#define _GNU_SOURCE

#include <stdlib.h>

#include <dlfcn.h>

#include "../../mc/mc_base.h"
#include "mmprivate.h"
#include "xbt_modinter.h"
#include "internal_config.h"
#include <math.h>
#include "../mc/mc_protocol.h"

//#define MM_LEGACY_VERBOSE 1 /* define this to see which version of malloc gets used */

/* The mmalloc() package can use a single implicit malloc descriptor
   for mmalloc/mrealloc/mfree operations which do not supply an explicit
   descriptor.  This allows mmalloc() to provide
   backwards compatibility with the non-mmap'd version. */
xbt_mheap_t __mmalloc_default_mdp = NULL;

static int __malloc_use_mmalloc;

int malloc_use_mmalloc(void)
{
  return __malloc_use_mmalloc;
}

static xbt_mheap_t __mmalloc_current_heap = NULL;     /* The heap we are currently using. */

xbt_mheap_t mmalloc_get_current_heap(void)
{
  return __mmalloc_current_heap;
}

void mmalloc_set_current_heap(xbt_mheap_t new_heap)
{
  __mmalloc_current_heap = new_heap;
}

#ifdef MMALLOC_WANT_OVERRIDE_LEGACY

/* Fake implementations, they are used to fool dlsym:
 * dlsym used calloc and falls back to some other mechanism
 * if this fails.
 */
static void* mm_fake_calloc(size_t nmemb, size_t size) { return NULL; }
static void* mm_fake_malloc(size_t n)                  { return NULL; }
static void* mm_fake_realloc(void *p, size_t s)        { return NULL; }

/* Function signatures for the main malloc functions: */
typedef void* (*mm_malloc_t)(size_t size);
typedef void  (*mm_free_t)(void*);
typedef void* (*mm_calloc_t)(size_t nmemb, size_t size);
typedef void* (*mm_realloc_t)(void *ptr, size_t size);

/* Function pointers to the real/next implementations: */
static mm_malloc_t mm_real_malloc   = mm_fake_malloc;
static mm_free_t mm_real_free;
static mm_calloc_t mm_real_calloc   = mm_fake_calloc;
static mm_realloc_t mm_real_realloc = mm_fake_realloc;

#define GET_HEAP() __mmalloc_current_heap

/** Constructor functions used to initialize the malloc implementation
 */
static void __attribute__((constructor(101))) mm_legacy_constructor()
{
  __malloc_use_mmalloc = getenv(MC_ENV_VARIABLE) ? 1 : 0;
  if (__malloc_use_mmalloc) {
    __mmalloc_current_heap = mmalloc_preinit();
  } else {
    mm_real_realloc  = (mm_realloc_t) dlsym(RTLD_NEXT, "realloc");
    mm_real_malloc   = (mm_malloc_t)  dlsym(RTLD_NEXT, "malloc");
    mm_real_free     = (mm_free_t)    dlsym(RTLD_NEXT, "free");
    mm_real_calloc   = (mm_calloc_t)  dlsym(RTLD_NEXT, "calloc");
  }
}

void *malloc(size_t n)
{
  if (!__malloc_use_mmalloc) {
    return mm_real_malloc(n);
  }

  xbt_mheap_t mdp = GET_HEAP();
  if (!mdp)
    return NULL;

  LOCK(mdp);
  void *ret = mmalloc(mdp, n);
  UNLOCK(mdp);
  return ret;
}

void *calloc(size_t nmemb, size_t size)
{
  if (!__malloc_use_mmalloc) {
    return mm_real_calloc(nmemb, size);
  }

  xbt_mheap_t mdp = GET_HEAP();
  if (!mdp)
    return NULL;

  LOCK(mdp);
  void *ret = mmalloc(mdp, nmemb*size);
  UNLOCK(mdp);
  // This was already done in the callee:
  if(!(mdp->options & XBT_MHEAP_OPTION_MEMSET)) {
    memset(ret, 0, nmemb * size);
  }
  return ret;
}

void *realloc(void *p, size_t s)
{
  if (!__malloc_use_mmalloc) {
    return mm_real_realloc(p, s);
  }

  xbt_mheap_t mdp = GET_HEAP();
  if (!mdp)
    return NULL;

  LOCK(mdp);
  void* ret = mrealloc(mdp, p, s);
  UNLOCK(mdp);
  return ret;
}

void free(void *p)
{
  if (!__malloc_use_mmalloc) {
    mm_real_free(p);
    return;
  }

  if (!p)
    return;

  xbt_mheap_t mdp = GET_HEAP();
  LOCK(mdp);
  mfree(mdp, p);
  UNLOCK(mdp);
}
#endif /* WANT_MALLOC_OVERRIDE */
