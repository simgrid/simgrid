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

/* Allocate SIZE bytes of memory.  */
extern void *mmalloc(void *md, size_t size);

/* Re-allocate the previously allocated block in void*, making the new block
   SIZE bytes long.  */
extern void *mrealloc(void *md, void *ptr, size_t size);

/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
extern void *mcalloc(void *md, size_t nmemb, size_t size);

/* Free a block allocated by `mmalloc', `mrealloc' or `mcalloc'.  */
extern void mfree(void *md, void *ptr);

/* Allocate SIZE bytes allocated to ALIGNMENT bytes.  */
extern void *mmemalign(void *md, size_t alignment, size_t size);

/* Allocate SIZE bytes on a page boundary.  */
extern void *mvalloc(void *md, size_t size);

extern void *mmalloc_attach(int fd, void *baseaddr);

extern void mmalloc_detach_no_free(void *md);

extern void *mmalloc_detach(void *md);

/* return the heap used when NULL is passed as first argument to any mm* function */
extern void *mmalloc_get_default_md(void);

extern void *mmalloc_findbase(int size);

extern int mmalloc_compare_heap(void *h1, void *h2, void *std_heap_addr);

extern void mmalloc_display_info_heap(void *h);

/* To change the heap used when using the legacy version malloc/free/realloc and such */
void mmalloc_set_current_heap(void *new_heap);
void *mmalloc_get_current_heap(void);



#endif                          /* MMALLOC_H */
