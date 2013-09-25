/* Copyright (c) 2010-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */

#include "mmprivate.h"
#include "internal_config.h"
#include <math.h>

//#define MM_LEGACY_VERBOSE 1 /* define this to see which version of malloc gets used */

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


#ifdef MMALLOC_WANT_OVERRIDE_LEGACY
#ifdef HAVE_GNU_LD

#undef _GNU_SOURCE
#define _GNU_SOURCE 1
#include <dlfcn.h>

static void * (*real_malloc) (size_t);
static void * (*real_realloc) (void*,size_t);
static void * (*real_free) (void*);

static void mm_gnuld_legacy_init(void) { /* This function is called from mmalloc_preinit(); it works even if it's static because all mm is in mm.c */
  real_malloc = (void * (*) (size_t)) dlsym(RTLD_NEXT, "malloc");
  real_realloc = (void * (*) (void*,size_t)) dlsym(RTLD_NEXT, "realloc");
  real_free = (void * (*) (void*)) dlsym(RTLD_NEXT, "free");
  __mmalloc_current_heap = __mmalloc_default_mdp;
}

/* Hello pimple!
 * DL needs some memory while resolving the malloc symbol, that is somehow problematic
 * To that extend, we have a little area here living in .BSS that we return if asked for memory before the malloc is resolved.
 */
int allocated_junk=0; /* keep track of whether our little area was already given to someone */
char junkarea[512];

/* This version use mmalloc if there is a current heap, or the legacy implem if not */
void *malloc(size_t n) {
  xbt_mheap_t mdp = __mmalloc_current_heap;
  void *ret;
#ifdef MM_LEGACY_VERBOSE
  static int warned_raw = 0;
  static int warned_mmalloc = 0;
#endif

  if (mdp) {
    LOCK(mdp);
    ret = mmalloc(mdp, n);
    UNLOCK(mdp);
#ifdef MM_LEGACY_VERBOSE
    if (!warned_mmalloc) {
      fprintf(stderr,"Using mmalloc; enabling the model-checker in cmake may have a bad impact on your simulation performance\n");
      warned_mmalloc = 1;
    }
#endif
  } else {
    if (!real_malloc) {
      if (allocated_junk) {
        fprintf(stderr,
            "Panic: real malloc symbol not resolved yet, and I already gave my little private memory chunk away. "
            "Damn LD, we must extend our code to have several such areas.\n");
        exit(1);
      } else if (n>512) {
        fprintf(stderr,
            "Panic: real malloc symbol not resolved yet, and I need %zu bytes while my little private memory chunk is only 512 bytes wide. "
            "Damn LD, we must fix our code to extend this area.\n",n);
        exit(1);
      } else {
        allocated_junk = 1;
        return junkarea;
      }
    }
#ifdef MM_LEGACY_VERBOSE
    if (!warned_raw) {
      fprintf(stderr,"Using system malloc after interception; you seem to be currently model-checking\n");
      warned_raw = 1;
    }
#endif
    ret = real_malloc(n);
  }
  return ret;
}


void *calloc(size_t nmemb, size_t size)
{
  void *ret = malloc(nmemb*size);
  memset(ret, 0, nmemb * size);
  return ret;
}

void *realloc(void *p, size_t s)
{
  xbt_mheap_t mdp = __mmalloc_current_heap;
  void *ret;

  if (mdp) {
    LOCK(mdp);
    ret = mrealloc(mdp, p, s);
    UNLOCK(mdp);
  } else {
    ret = real_realloc(p,s);
  }

  return ret;
}

void free(void *p)
{
  if (p==NULL)
    return;
  if (p!=junkarea) {
    xbt_mheap_t mdp = __mmalloc_current_heap;

    if (mdp) {
      LOCK(mdp);
      mfree(mdp, p);
      UNLOCK(mdp);
    } else {
      real_free(p);
    }
  } else {
    allocated_junk=0;
  }
}


#else /* NO GNU_LD */
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
  if (p != NULL) {
    xbt_mheap_t mdp = __mmalloc_current_heap ?: (xbt_mheap_t) mmalloc_preinit();

    LOCK(mdp);
    mfree(mdp, p);
    UNLOCK(mdp);
  }
}
#endif /* NO GNU_LD */
#endif /* WANT_MALLOC_OVERRIDE */


