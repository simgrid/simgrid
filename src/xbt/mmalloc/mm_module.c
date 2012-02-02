/* Initialization for access to a mmap'd malloc managed region.
   Copyright 1992, 2000 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <fcntl.h>              /* After sys/types.h, at least for dpx/2.  */
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>             /* Prototypes for lseek */
#endif
#include "mmprivate.h"
#include "xbt/ex.h"
#include "xbt_modinter.h" /* declarations of mmalloc_preinit and friends that live here */

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

    	  struct mdesc mtemp;
    	  struct mdesc *mdp = NULL, *mdptemp = NULL;

    	  if (lseek(fd, 0L, SEEK_SET) != 0)
    	    return NULL;
    	  if (read(fd, (char *) &mtemp, sizeof(mtemp)) != sizeof(mtemp))
    	    return NULL;
    	  if (mtemp.headersize != sizeof(mtemp))
    	    return NULL;
    	  if (strcmp(mtemp.magic, MMALLOC_MAGIC) != 0)
    	    return NULL;
    	  if (mtemp.version > MMALLOC_VERSION)
    	    return NULL;

    	  mtemp.fd = fd;
    	  if (__mmalloc_remap_core(&mtemp) == mtemp.base) {
    	    mdp = (struct mdesc *) mtemp.base;
    	    mdp->fd = fd;
    	    if(!mdp->refcount){
    	      sem_init(&mdp->sem, 1, 1);
    	      mdp->refcount++;
    	    }
    	  }

    	  /* Add the new heap to the linked list of heaps attached by mmalloc */
    	  mdptemp = __mmalloc_default_mdp;
    	  while(mdptemp->next_mdesc)
    	    mdptemp = mdptemp->next_mdesc;

    	  LOCK(mdptemp);
    	    mdptemp->next_mdesc = mdp;
    	  UNLOCK(mdptemp);

    	  return (mdp);
    }
  }

  /* If the user provided NULL BASEADDR then fail */
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
  
  /* If we have not been passed a valid open file descriptor for the file
     to map to, then we go for an anonymous map */

  if (mdp->fd < 0){
    mdp->flags |= MMALLOC_ANONYMOUS;
    sem_init(&mdp->sem, 0, 1);
  }else{
    sem_init(&mdp->sem, 1, 1);
  }
  
  /* If we have not been passed a valid open file descriptor for the file
     to map to, then open /dev/zero and use that to map to. */

  /* Now try to map in the first page, copy the malloc descriptor structure
     there, and arrange to return a pointer to this new copy.  If the mapping
     fails, then close the file descriptor if it was opened by us, and arrange
     to return a NULL. */

  if ((mbase = mmorecore(mdp, sizeof(mtemp))) != NULL) {
    memcpy(mbase, mdp, sizeof(mtemp));
  } else {
    THROWF(system_error,0,"morecore failed to get some memory!");
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
    LOCK(mdp) ;
    sem_destroy(&mdp->sem);
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

    if ((mmorecore(&mtemp,
                        (char *) mtemp.base - (char *) mtemp.breakval)) ==
        NULL) {
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
    unsigned long mask = ~((unsigned long)getpagesize() - 1);
    void *addr = (void*)(((unsigned long)sbrk(0) + HEAP_OFFSET) & mask);
    __mmalloc_default_mdp = xbt_mheap_new(-1, addr);
    /* Fixme? only the default mdp in protected against forks */
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
  /* Do not detach the default mdp or ldl won't be able to free the memory it allocated since we're in memory */
  //  mmalloc_detach(__mmalloc_default_mdp);
  xbt_mheap_destroy_no_free(__mmalloc_default_mdp);
}
