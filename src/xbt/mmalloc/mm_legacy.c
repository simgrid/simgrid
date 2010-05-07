/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Redefine the classical malloc/free/realloc functions so that they fit well in the mmalloc framework */

#include "mmprivate.h"

static void *__mmalloc_current_heap=NULL; /* The heap we are currently using. */

void* mmalloc_get_current_heap(void) {
  return __mmalloc_current_heap;
}
void mmalloc_set_current_heap(void *new_heap) {
  __mmalloc_current_heap=new_heap;
}

#define MMALLOC_WANT_OVERIDE_LEGACY /* comment this when stuff goes horribly bad around memory allocation */
#ifdef MMALLOC_WANT_OVERIDE_LEGACY
void *malloc(size_t n) {
  void *ret = mmalloc(__mmalloc_current_heap, n);

  return ret;
}

void *calloc(size_t nmemb, size_t size) {
  size_t total_size = nmemb * size;

  void *ret = mmalloc(__mmalloc_current_heap, total_size);

  /* Fill the allocated memory with zeroes to mimic calloc behaviour */
  memset(ret,'\0', total_size);

  return ret;
}

void *realloc(void *p, size_t s) {
  void *ret = NULL;

  if (s) {
    if (p)
      ret = mrealloc(__mmalloc_current_heap, p,s);
    else
      ret = mmalloc(__mmalloc_current_heap,s);
  } else {
    if (p)
      mfree(__mmalloc_current_heap,p);
  }

  return ret;
}

void free(void *p) {
  return mfree(__mmalloc_current_heap, p);
}
#endif

/* Make sure it works with md==NULL */

#define HEAP_OFFSET   40960000    /* Safety gap from the heap's break address */

void *mmalloc_get_default_md(void) {
  xbt_assert(__mmalloc_default_mdp);
  return __mmalloc_default_mdp;
}

/* Initialize the default malloc descriptor. */
#include "xbt_modinter.h"
void mmalloc_preinit(void) {
  __mmalloc_default_mdp = mmalloc_attach(-1, (char *)sbrk(0) + HEAP_OFFSET);
  xbt_assert(__mmalloc_default_mdp != NULL);
}
void mmalloc_postexit(void) {
  /* Do not detach the default mdp or ldl won't be able to free the memory it allocated since we're in memory */
  //  mmalloc_detach(__mmalloc_default_mdp);
}
