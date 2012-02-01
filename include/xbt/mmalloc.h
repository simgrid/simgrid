/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
   This file was then part of the GNU C Library. */

/* Copyright (c) 2010-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef MMALLOC_H
#define MMALLOC_H 1

#ifdef HAVE_STDDEF_H
#  include <stddef.h>
#else
#  include <sys/types.h>        /* for size_t */
#  include <stdio.h>            /* for NULL */
#endif

/* Datatype representing a separate heap. The whole point of the mmalloc module
 * is to allow several such heaps in the process. It thus works by redefining
 * all the classical memory management functions (malloc and friends) with an
 * extra first argument: the heap in which the memory is to be taken.
 *
 * The heap structure itself is an opaque object that shouldnt be messed with.
 */
typedef struct mdesc *xbt_mheap_t;

/* Allocate SIZE bytes of memory.  */
extern void *mmalloc(xbt_mheap_t md, size_t size);

/* Re-allocate the previously allocated block in void*, making the new block
   SIZE bytes long.  */
extern void *mrealloc(xbt_mheap_t md, void *ptr, size_t size);

/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
extern void *mcalloc(xbt_mheap_t md, size_t nmemb, size_t size);

/* Free a block allocated by `mmalloc', `mrealloc' or `mcalloc'.  */
extern void mfree(xbt_mheap_t md, void *ptr);

/* Allocate SIZE bytes allocated to ALIGNMENT bytes.  */
extern void *mmemalign(xbt_mheap_t md, size_t alignment, size_t size);

/* Allocate SIZE bytes on a page boundary.  */
extern void *mvalloc(xbt_mheap_t md, size_t size);

extern xbt_mheap_t mmalloc_attach(int fd, void *baseaddr);

extern void mmalloc_detach_no_free(xbt_mheap_t md);

extern void *mmalloc_detach(xbt_mheap_t md);

/* return the heap used when NULL is passed as first argument to any mm* function */
extern xbt_mheap_t mmalloc_get_default_md(void);

extern void *mmalloc_findbase(int size);

extern void mmalloc_display_info_heap(xbt_mheap_t h);

/* To change the heap used when using the legacy version malloc/free/realloc and such */
void mmalloc_set_current_heap(xbt_mheap_t new_heap);
xbt_mheap_t mmalloc_get_current_heap(void);

int mmalloc_compare_heap(xbt_mheap_t mdp1, xbt_mheap_t mdp2, void *std_heap_addr);


#endif                          /* MMALLOC_H */
