/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
   This file was then part of the GNU C Library. */

#ifndef SIMGRID_MMALLOC_H
#define SIMGRID_MMALLOC_H 1

#include "src/internal_config.h"
#if HAVE_MMALLOC

# include <sys/types.h>        /* for size_t */
# include <stdio.h>            /* for NULL */

#include "xbt/dynar.h"
#include "xbt/dict.h"

SG_BEGIN_DECL()

/* Datatype representing a separate heap. The whole point of the mmalloc module is to allow several such heaps in the
 * process. It thus works by redefining all the classical memory management functions (malloc and friends) with an
 * extra first argument: the heap in which the memory is to be taken.
 *
 * The heap structure itself is an opaque object that shouldnt be messed with.
 */
typedef struct mdesc s_xbt_mheap_t;
typedef struct mdesc *xbt_mheap_t;

/* Allocate SIZE bytes of memory (and memset it to 0).  */
XBT_PUBLIC( void ) *mmalloc(xbt_mheap_t md, size_t size);

/* Allocate SIZE bytes of memory (and don't mess with it) */
void *mmalloc_no_memset(xbt_mheap_t mdp, size_t size);

/* Re-allocate the previously allocated block in void*, making the new block SIZE bytes long.  */
XBT_PUBLIC( void ) *mrealloc(xbt_mheap_t md, void *ptr, size_t size);

/* Free a block allocated by `mmalloc', `mrealloc' or `mcalloc'.  */
XBT_PUBLIC( void ) mfree(xbt_mheap_t md, void *ptr);

XBT_PUBLIC( xbt_mheap_t ) xbt_mheap_new(int fd, void *baseaddr);

#define XBT_MHEAP_OPTION_MEMSET 1

XBT_PUBLIC( xbt_mheap_t ) xbt_mheap_new_options(int fd, void *baseaddr, int options);

XBT_PUBLIC( void ) xbt_mheap_destroy_no_free(xbt_mheap_t md);

XBT_PUBLIC( void ) *xbt_mheap_destroy(xbt_mheap_t md);

/* return the heap used when NULL is passed as first argument to any mm* function */
XBT_PUBLIC( xbt_mheap_t ) mmalloc_get_default_md(void);

/* To change the heap used when using the legacy version malloc/free/realloc and such */
xbt_mheap_t mmalloc_set_current_heap(xbt_mheap_t new_heap);
xbt_mheap_t mmalloc_get_current_heap(void);

size_t mmalloc_get_bytes_used(xbt_mheap_t);
ssize_t mmalloc_get_busy_size(xbt_mheap_t, void *ptr);

void* malloc_no_memset(size_t n);

SG_END_DECL()

#endif
#endif                          /* SIMGRID_MMALLOC_H */
