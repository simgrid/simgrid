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

#include "xbt/dynar.h"

/* Datatype representing a separate heap. The whole point of the mmalloc module
 * is to allow several such heaps in the process. It thus works by redefining
 * all the classical memory management functions (malloc and friends) with an
 * extra first argument: the heap in which the memory is to be taken.
 *
 * The heap structure itself is an opaque object that shouldnt be messed with.
 */
typedef struct mdesc *xbt_mheap_t;

/* Allocate SIZE bytes of memory (and memset it to 0).  */
XBT_PUBLIC( void ) *mmalloc(xbt_mheap_t md, size_t size);

/* Allocate SIZE bytes of memory (and don't mess with it) */
void *mmalloc_no_memset(xbt_mheap_t mdp, size_t size);

/* Re-allocate the previously allocated block in void*, making the new block
   SIZE bytes long.  */
XBT_PUBLIC( void ) *mrealloc(xbt_mheap_t md, void *ptr, size_t size);

/* Free a block allocated by `mmalloc', `mrealloc' or `mcalloc'.  */
XBT_PUBLIC( void ) mfree(xbt_mheap_t md, void *ptr);

/* Allocate SIZE bytes allocated to ALIGNMENT bytes.  */
XBT_PUBLIC( void ) *mmemalign(xbt_mheap_t md, size_t alignment, size_t size);

/* Allocate SIZE bytes on a page boundary.  */
XBT_PUBLIC( void ) *mvalloc(xbt_mheap_t md, size_t size);

XBT_PUBLIC( xbt_mheap_t ) xbt_mheap_new(int fd, void *baseaddr);

XBT_PUBLIC( void ) xbt_mheap_destroy_no_free(xbt_mheap_t md);

XBT_PUBLIC( void ) *xbt_mheap_destroy(xbt_mheap_t md);

/* return the heap used when NULL is passed as first argument to any mm* function */
XBT_PUBLIC( xbt_mheap_t ) mmalloc_get_default_md(void);

/* To change the heap used when using the legacy version malloc/free/realloc and such */
void mmalloc_set_current_heap(xbt_mheap_t new_heap);
xbt_mheap_t mmalloc_get_current_heap(void);

int mmalloc_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2, xbt_dynar_t *stacks1, xbt_dynar_t *stacks2, xbt_dynar_t equals);
int mmalloc_linear_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2);
void init_heap_information(xbt_mheap_t heap1, xbt_mheap_t heap2);
void match_equals(xbt_dynar_t list, xbt_dynar_t equals);
int compare_area(void *area1, void* area2, xbt_dynar_t previous, xbt_dynar_t equals);

void mmalloc_backtrace_block_display(void* heapinfo, int block);
void mmalloc_backtrace_fragment_display(void* heapinfo, int block, int frag);
void mmalloc_backtrace_display(void *addr);

int is_free_area(void *area, xbt_mheap_t heap);

size_t mmalloc_get_chunks_used(xbt_mheap_t);



#endif                          /* MMALLOC_H */
