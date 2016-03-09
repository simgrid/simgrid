/* Initialization for acces s to a mmap'd malloc managed region. */

/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright 1992, 2000 Free Software Foundation, Inc.

   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com

   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If
   not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "src/internal_config.h"
#include <sys/types.h>
#include <fcntl.h>              /* After sys/types.h, at least for dpx/2.  */
#include <sys/stat.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>             /* Prototypes for lseek */
#endif
#include "mmprivate.h"
#include "xbt/ex.h"
#include "src/xbt_modinter.h" /* declarations of mmalloc_preinit and friends that live here */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

/* Initialize access to a mmalloc managed region.

   If FD is a valid file descriptor for an open file then data for the
   mmalloc managed region is mapped to that file, otherwise an anonymous
   map is used if supported by the underlying OS. In case of running in
   an OS without support of anonymous mappings then "/dev/zero" is used 
   and in both cases the data will not exist in any filesystem object.

   If the open file corresponding to FD is from a previous use of
   mmalloc and passes some basic sanity checks to ensure that it is
   compatible with the current mmalloc package, then its data is
   mapped in and is immediately accessible at the same addresses in
   the current process as the process that created the file (ignoring
   the BASEADDR parameter).

   For non valid FDs or empty files ones the mapping is established 
   starting at the specified address BASEADDR in the process address 
   space.

   The provided BASEADDR should be choosed carefully in order to avoid
   bumping into existing mapped regions or future mapped regions.

   On success, returns a "malloc descriptor" which is used in subsequent
   calls to other mmalloc package functions.  It is explicitly "void *"
   so that users of the package don't have to worry about the actual
   implementation details.

   On failure returns NULL. */

xbt_mheap_t xbt_mheap_new(int fd, void *baseaddr)
{
  return xbt_mheap_new_options(fd, baseaddr, 0);
}

xbt_mheap_t xbt_mheap_new_options(int fd, void *baseaddr, int options)
{
  struct mdesc mtemp;
  xbt_mheap_t mdp;
  void *mbase;
  struct stat sbuf;

  /* First check to see if FD is a valid file descriptor, and if so, see
     if the file has any current contents (size > 0).  If it does, then
     attempt to reuse the file.  If we can't reuse the file, either
     because it isn't a valid mmalloc produced file, was produced by an
     obsolete version, or any other reason, then we fail to attach to
     this file. */

  if (fd >= 0) {
    if (fstat(fd, &sbuf) < 0)
      return (NULL);

    else if (sbuf.st_size > 0) {
      /* We were given an valid file descriptor on an open file, so try to remap
         it into the current process at the same address to which it was previously
         mapped. It naturally have to pass some sanity checks for that.

         Note that we have to update the file descriptor number in the malloc-
         descriptor read from the file to match the current valid one, before
         trying to map the file in, and again after a successful mapping and
         after we've switched over to using the mapped in malloc descriptor
         rather than the temporary one on the stack.

         Once we've switched over to using the mapped in malloc descriptor, we
         have to update the pointer to the morecore function, since it almost
         certainly will be at a different address if the process reusing the
         mapped region is from a different executable.

         Also note that if the heap being remapped previously used the mmcheckf()
         routines, we need to update the hooks since their target functions
         will have certainly moved if the executable has changed in any way.
         We do this by calling mmcheckf() internally.

         Returns a pointer to the malloc descriptor if successful, or NULL if
         unsuccessful for some reason. */

      struct mdesc newmd;
      struct mdesc *mdptr = NULL, *mdptemp = NULL;

      if (lseek(fd, 0L, SEEK_SET) != 0)
        return NULL;
      if (read(fd, (char *) &newmd, sizeof(newmd)) != sizeof(newmd))
        return NULL;
      if (newmd.headersize != sizeof(newmd))
        return NULL;
      if (strcmp(newmd.magic, MMALLOC_MAGIC) != 0)
        return NULL;
      if (newmd.version > MMALLOC_VERSION)
        return NULL;

      newmd.fd = fd;
      if (__mmalloc_remap_core(&newmd) == newmd.base) {
        mdptr = (struct mdesc *) newmd.base;
        mdptr->fd = fd;
        if(!mdptr->refcount){
          pthread_mutex_init(&mdptr->mutex, NULL);
          mdptr->refcount++;
        }
      }

      /* Add the new heap to the linked list of heaps attached by mmalloc */
      mdptemp = __mmalloc_default_mdp;
      while(mdptemp->next_mdesc)
        mdptemp = mdptemp->next_mdesc;

      LOCK(mdptemp);
      mdptemp->next_mdesc = mdptr;
      UNLOCK(mdptemp);

      return mdptr;
    }
  }

  /* NULL is not a valid baseaddr as we cannot map anything there.
     C'mon, user. Think! */
  if (baseaddr == NULL)
    return (NULL);

  /* We start off with the malloc descriptor allocated on the stack, until
     we build it up enough to call _mmalloc_mmap_morecore() to allocate the
     first page of the region and copy it there.  Ensure that it is zero'd and
     then initialize the fields that we know values for. */

  mdp = &mtemp;
  memset((char *) mdp, 0, sizeof(mtemp));
  strncpy(mdp->magic, MMALLOC_MAGIC, MMALLOC_MAGIC_SIZE);
  mdp->headersize = sizeof(mtemp);
  mdp->version = MMALLOC_VERSION;
  mdp->fd = fd;
  mdp->base = mdp->breakval = mdp->top = baseaddr;
  mdp->next_mdesc = NULL;
  mdp->refcount = 1;
  mdp->options = options;
  
  /* If we have not been passed a valid open file descriptor for the file
     to map to, then we go for an anonymous map */

  if (mdp->fd < 0){
    mdp->flags |= MMALLOC_ANONYMOUS;
  }
  pthread_mutex_init(&mdp->mutex, NULL);
  /* If we have not been passed a valid open file descriptor for the file
     to map to, then open /dev/zero and use that to map to. */

  /* Now try to map in the first page, copy the malloc descriptor structure
     there, and arrange to return a pointer to this new copy.  If the mapping
     fails, then close the file descriptor if it was opened by us, and arrange
     to return a NULL. */

  if ((mbase = mmorecore(mdp, sizeof(mtemp))) != NULL) {
    memcpy(mbase, mdp, sizeof(mtemp));
  } else {
    fprintf(stderr, "morecore failed to get some more memory!\n");
    abort();
  }

  /* Add the new heap to the linked list of heaps attached by mmalloc */  
  if(__mmalloc_default_mdp){
    mdp = __mmalloc_default_mdp;
    while(mdp->next_mdesc)
      mdp = mdp->next_mdesc;

    LOCK(mdp);
    mdp->next_mdesc = (struct mdesc *)mbase;
    UNLOCK(mdp);
  }

  return mbase;
}



/** Terminate access to a mmalloc managed region, but do not free its content.
 *
 * This is for example useful for the base region where ldl stores its data
 *   because it leaves the place after us.
 */
void xbt_mheap_destroy_no_free(xbt_mheap_t md)
{
  struct mdesc *mdp = md;

  if(--mdp->refcount == 0){
    pthread_mutex_destroy(&mdp->mutex);
  }
}

/** Terminate access to a mmalloc managed region by unmapping all memory pages
    associated with the region, and closing the file descriptor if it is one
    that we opened.

    Returns NULL on success.

    Returns the malloc descriptor on failure, which can subsequently be used
    for further action, such as obtaining more information about the nature of
    the failure.

    Note that the malloc descriptor that we are using is currently located in
    region we are about to unmap, so we first make a local copy of it on the
    stack and use the copy. */

void *xbt_mheap_destroy(xbt_mheap_t mdp)
{
  struct mdesc mtemp, *mdptemp;

  if (mdp != NULL) {
    /* Remove the heap from the linked list of heaps attached by mmalloc */
    mdptemp = __mmalloc_default_mdp;
    while(mdptemp->next_mdesc != mdp )
      mdptemp = mdptemp->next_mdesc;

    mdptemp->next_mdesc = mdp->next_mdesc;

    xbt_mheap_destroy_no_free(mdp);
    mtemp = *mdp;

    /* Now unmap all the pages associated with this region by asking for a
       negative increment equal to the current size of the region. */

    if (mmorecore(&mtemp, (char *)mtemp.base - (char *)mtemp.breakval) == NULL) {
      /* Deallocating failed.  Update the original malloc descriptor
         with any changes */
      *mdp = mtemp;
    } else {
      if (mtemp.flags & MMALLOC_DEVZERO) {
        close(mtemp.fd);
      }
      mdp = NULL;
    }
  }

  return (mdp);
}

/* Safety gap from the heap's break address.
 * Try to increase this first if you experience strange errors under
 * valgrind. */
#define HEAP_OFFSET   (128UL<<20)

xbt_mheap_t mmalloc_get_default_md(void)
{
  xbt_assert(__mmalloc_default_mdp);
  return __mmalloc_default_mdp;
}

static void mmalloc_fork_prepare(void)
{
  xbt_mheap_t mdp = NULL;
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
  xbt_mheap_t mdp = NULL;
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
void *mmalloc_preinit(void)
{
  int res;
  if (__mmalloc_default_mdp == NULL) {
    if(!xbt_pagesize)
      xbt_pagesize = getpagesize();
    unsigned long mask = ~((unsigned long)xbt_pagesize - 1);
    void *addr = (void*)(((unsigned long)sbrk(0) + HEAP_OFFSET) & mask);
    __mmalloc_default_mdp = xbt_mheap_new_options(-1, addr, XBT_MHEAP_OPTION_MEMSET);
    /* Fixme? only the default mdp in protected against forks */
    // This is mandated to protect the mmalloced areas through forks. Think of tesh.
    // Nah, removing the mutex isn't a good idea either for tesh
    res = xbt_os_thread_atfork(mmalloc_fork_prepare,  
                               mmalloc_fork_parent, mmalloc_fork_child);
    if (res != 0)
      THROWF(system_error,0,"xbt_os_thread_atfork() failed: return value %d",res);
  }
  xbt_assert(__mmalloc_default_mdp != NULL);

  return __mmalloc_default_mdp;
}

void mmalloc_postexit(void)
{
  /* Do not destroy the default mdp or ldl won't be able to free the memory it
   * allocated since we're in memory */
  // xbt_mheap_destroy_no_free(__mmalloc_default_mdp);
}

// This is the underlying implementation of mmalloc_get_bytes_used_remote.
// Is it used directly in order to evaluate the bytes used from a different
// process.
size_t mmalloc_get_bytes_used_remote(size_t heaplimit, const malloc_info* heapinfo)
{
  int bytes = 0;
  for (size_t i=0; i<=heaplimit; ++i){
    if (heapinfo[i].type == MMALLOC_TYPE_UNFRAGMENTED){
      if (heapinfo[i].busy_block.busy_size > 0)
        bytes += heapinfo[i].busy_block.busy_size;
    } else if (heapinfo[i].type > 0) {
      for (size_t j=0; j < (size_t) (BLOCKSIZE >> heapinfo[i].type); j++){
        if(heapinfo[i].busy_frag.frag_size[j] > 0)
          bytes += heapinfo[i].busy_frag.frag_size[j];
      }
    }
  }
  return bytes;
}

size_t mmalloc_get_bytes_used(const xbt_mheap_t heap){
  const struct mdesc* heap_data = (const struct mdesc *) heap;
  return mmalloc_get_bytes_used_remote(heap_data->heaplimit, heap_data->heapinfo);
}

ssize_t mmalloc_get_busy_size(xbt_mheap_t heap, void *ptr){

  ssize_t block = ((char*)ptr - (char*)(heap->heapbase)) / BLOCKSIZE + 1;
  if(heap->heapinfo[block].type < 0)
    return -1;
  else if(heap->heapinfo[block].type == MMALLOC_TYPE_UNFRAGMENTED)
    return heap->heapinfo[block].busy_block.busy_size;
  else{
    ssize_t frag = ((uintptr_t) (ADDR2UINT (ptr) % (BLOCKSIZE))) >> heap->heapinfo[block].type;
    return heap->heapinfo[block].busy_frag.frag_size[frag];
  }
    
}

void mmcheck(xbt_mheap_t heap) {return;
  if (!heap->heapinfo)
    return;
  malloc_info* heapinfo = NULL;
  for (size_t i=1; i < heap->heaplimit; i += mmalloc_get_increment(heapinfo)) {
    heapinfo = heap->heapinfo + i;
    switch (heapinfo->type) {
    case MMALLOC_TYPE_HEAPINFO:
    case MMALLOC_TYPE_FREE:
      if (heapinfo->free_block.size==0) {
        xbt_die("Block size == 0");
      }
      break;
    case MMALLOC_TYPE_UNFRAGMENTED:
      if (heapinfo->busy_block.size==0) {
        xbt_die("Block size == 0");
      }
      if (heapinfo->busy_block.busy_size==0 && heapinfo->busy_block.size!=0) {
        xbt_die("Empty busy block");
      }
      break;
    default:
      if (heapinfo->type<0) {
        xbt_die("Unkown mmalloc block type.");
      }
    }
  }
}
