/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */
#define _GNU_SOURCE

#include <stdlib.h>

#include <dlfcn.h>

#include "src/mc/mc_base.h"
#include "mmprivate.h"
#include "src/xbt_modinter.h"
#include "src/internal_config.h"
#include <math.h>
#include "src/mc/mc_protocol.h"

/* ***** Whether to use `mmalloc` of the undrlying malloc ***** */

static int __malloc_use_mmalloc;

int malloc_use_mmalloc(void)
{
  return __malloc_use_mmalloc;
}

/* ***** Current heap ***** */

/* The mmalloc() package can use a single implicit malloc descriptor
   for mmalloc/mrealloc/mfree operations which do not supply an explicit
   descriptor.  This allows mmalloc() to provide
   backwards compatibility with the non-mmap'd version. */
xbt_mheap_t __mmalloc_default_mdp = NULL;

/* The heap we are currently using. */
static xbt_mheap_t __mmalloc_current_heap = NULL;

xbt_mheap_t mmalloc_get_current_heap(void)
{
  return __mmalloc_current_heap;
}

xbt_mheap_t mmalloc_set_current_heap(xbt_mheap_t new_heap)
{
  xbt_mheap_t heap = __mmalloc_current_heap;
  __mmalloc_current_heap = new_heap;
  return heap;
}

/* Override the malloc-like functions if MC is activated at compile time */
#if HAVE_MC

/* ***** Temporary allocator
 *
 * This is used before we have found the real malloc implementation with dlsym.
 */

#define BUFFER_SIZE 32
static size_t fake_alloc_index;
static uint64_t buffer[BUFFER_SIZE];

/* Fake implementations, they are used to fool dlsym:
 * dlsym used calloc and falls back to some other mechanism
 * if this fails.
 */
static void* mm_fake_malloc(size_t n)
{
  // How many uint64_t do w need?
  size_t count = n / sizeof(uint64_t);
  if (n % sizeof(uint64_t))
    count++;
  // Check that we have enough availabel memory:
  if (fake_alloc_index + count >= BUFFER_SIZE)
    exit(127);
  // Allocate it:
  uint64_t* res = buffer + fake_alloc_index;
  fake_alloc_index += count;
  return res;
}

static void* mm_fake_calloc(size_t nmemb, size_t size)
{
  // This is fresh .bss data, we don't need to clear it:
  size_t n = nmemb * size;
  return mm_fake_malloc(n);
}

static void* mm_fake_realloc(void *p, size_t s)
{
  return mm_fake_malloc(s);
}

static void mm_fake_free(void *p)
{
}

/* Function signatures for the main malloc functions: */
typedef void* (*mm_malloc_t)(size_t size);
typedef void  (*mm_free_t)(void*);
typedef void* (*mm_calloc_t)(size_t nmemb, size_t size);
typedef void* (*mm_realloc_t)(void *ptr, size_t size);

/* Function pointers to the real/next implementations: */
static mm_malloc_t mm_real_malloc;
static mm_free_t mm_real_free;
static mm_calloc_t mm_real_calloc;
static mm_realloc_t mm_real_realloc;

static int mm_initializing;
static int mm_initialized;

/** Constructor functions used to initialize the malloc implementation
 */
static void __attribute__((constructor(101))) mm_legacy_constructor()
{
  if (mm_initialized)
    return;
  mm_initializing = 1;
  __malloc_use_mmalloc = getenv(MC_ENV_VARIABLE) ? 1 : 0;
  if (__malloc_use_mmalloc) {
    __mmalloc_current_heap = mmalloc_preinit();
  } else {
    mm_real_realloc  = dlsym(RTLD_NEXT, "realloc");
    mm_real_malloc   = dlsym(RTLD_NEXT, "malloc");
    mm_real_free     = dlsym(RTLD_NEXT, "free");
    mm_real_calloc   = dlsym(RTLD_NEXT, "calloc");
  }
  mm_initializing = 0;
  mm_initialized = 1;
}

/* ***** malloc/free implementation
 *
 * They call either the underlying/native/RTLD_NEXT implementation (non MC mode)
 * or the mm implementation (MC mode).
 *
 * If we are initializing the malloc subsystem, we call the fake/dummy `malloc`
 * implementation. This is necessary because `dlsym` calls `malloc` and friends.
 */

#define GET_HEAP() __mmalloc_current_heap

void* malloc_no_memset(size_t n)
{
  if (!mm_initialized) {
    if (mm_initializing)
      return mm_fake_malloc(n);
    mm_legacy_constructor();
  }

  if (!__malloc_use_mmalloc) {
    return mm_real_malloc(n);
  }

  xbt_mheap_t mdp = GET_HEAP();
  if (!mdp)
    return NULL;

  LOCK(mdp);
  void *ret = mmalloc_no_memset(mdp, n);
  UNLOCK(mdp);
  return ret;
}

void *malloc(size_t n)
{
  if (!mm_initialized) {
    if (mm_initializing)
      return mm_fake_malloc(n);
    mm_legacy_constructor();
  }

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
  if (!mm_initialized) {
    if (mm_initializing)
      return mm_fake_calloc(nmemb, size);
    mm_legacy_constructor();
  }

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
  if (!mm_initialized) {
    if (mm_initializing)
      return mm_fake_realloc(p, s);
    mm_legacy_constructor();
  }

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
  if (!mm_initialized) {
    if (mm_initializing)
      return mm_fake_free(p);
    mm_legacy_constructor();
  }

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
#endif /* HAVE_MC */
