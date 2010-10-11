/* Standard debugging hooks for `mmalloc'.
   Copyright 1990, 1991, 1992 Free Software Foundation

   Written May 1989 by Mike Haertel.
   Heavily modified Mar 1992 by Fred Fish (fnf@cygnus.com) */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mmprivate.h"

/* Default function to call when something awful happens.  The application
   can specify an alternate function to be called instead (and probably will
   want to). */

extern void abort(void);

/* Arbitrary magical numbers.  */

#define MAGICWORD	(unsigned int) 0xfedabeeb       /* Active chunk */
#define MAGICWORDFREE	(unsigned int) 0xdeadbeef       /* Inactive chunk */
#define MAGICBYTE	((char) 0xd7)

/* Each memory allocation is bounded by a header structure and a trailer
   byte.  I.E.

	<size><magicword><user's allocation><magicbyte>

   The pointer returned to the user points to the first byte in the
   user's allocation area.  The magic word can be tested to detect
   buffer underruns and the magic byte can be tested to detect overruns. */

struct hdr {
  size_t size;                  /* Exact size requested by user.  */
  unsigned long int magic;      /* Magic number to check header integrity.  */
};

static void checkhdr(struct mdesc *mdp, const struct hdr *hdr);
static void mfree_check(void *md, void *ptr);
static void *mmalloc_check(void *md, size_t size);
static void *mrealloc_check(void *md, void *ptr, size_t size);

/* Check the magicword and magicbyte, and if either is corrupted then
   call the emergency abort function specified for the heap in use. */

static void checkhdr(struct mdesc *mdp, const struct hdr *hdr)
{
  if (hdr->magic != MAGICWORD ||
      ((char *) &hdr[1])[hdr->size] != MAGICBYTE) {
    (*mdp->abortfunc) ();
  }
}

static void mfree_check(void *md, void *ptr)
{
  struct hdr *hdr = ((struct hdr *) ptr) - 1;
  struct mdesc *mdp;

  mdp = MD_TO_MDP(md);
  checkhdr(mdp, hdr);
  hdr->magic = MAGICWORDFREE;
  mdp->mfree_hook = NULL;
  mfree(md, (void *) hdr);
  mdp->mfree_hook = mfree_check;
}

static void *mmalloc_check(void *md, size_t size)
{
  struct hdr *hdr;
  struct mdesc *mdp;
  size_t nbytes;

  mdp = MD_TO_MDP(md);
  mdp->mmalloc_hook = NULL;
  nbytes = sizeof(struct hdr) + size + 1;
  hdr = (struct hdr *) mmalloc(md, nbytes);
  mdp->mmalloc_hook = mmalloc_check;
  if (hdr != NULL) {
    hdr->size = size;
    hdr->magic = MAGICWORD;
    hdr++;
    *((char *) hdr + size) = MAGICBYTE;
  }
  return ((void *) hdr);
}

static void *mrealloc_check(void *md, void *ptr, size_t size)
{
  struct hdr *hdr = ((struct hdr *) ptr) - 1;
  struct mdesc *mdp;
  size_t nbytes;

  mdp = MD_TO_MDP(md);
  checkhdr(mdp, hdr);
  mdp->mfree_hook = NULL;
  mdp->mmalloc_hook = NULL;
  mdp->mrealloc_hook = NULL;
  nbytes = sizeof(struct hdr) + size + 1;
  hdr = (struct hdr *) mrealloc(md, (void *) hdr, nbytes);
  mdp->mfree_hook = mfree_check;
  mdp->mmalloc_hook = mmalloc_check;
  mdp->mrealloc_hook = mrealloc_check;
  if (hdr != NULL) {
    hdr->size = size;
    hdr++;
    *((char *) hdr + size) = MAGICBYTE;
  }
  return ((void *) hdr);
}

/* Turn on default checking for mmalloc/mrealloc/mfree, for the heap specified
   by MD.  If FUNC is non-NULL, it is a pointer to the function to call
   to abort whenever memory corruption is detected.  By default, this is the
   standard library function abort().

   Note that we disallow installation of initial checking hooks if mmalloc
   has been called at any time for this particular heap, since if any region
   that is allocated prior to installation of the hooks is subsequently
   reallocated or freed after installation of the hooks, it is guaranteed
   to trigger a memory corruption error.  We do this by checking the state
   of the MMALLOC_INITIALIZED flag.  If the FORCE argument is non-zero, this
   checking is disabled and it is allowed to install the checking hooks at any
   time.  This is useful on systems where the C runtime makes one or more
   malloc calls before the user code had a chance to call mmcheck or mmcheckf,
   but never calls free with these values.  Thus if we are certain that only
   values obtained from mallocs after an mmcheck/mmcheckf will ever be passed
   to free(), we can go ahead and force installation of the useful checking
   hooks.

   However, we can call this function at any time after the initial call,
   to update the function pointers to the checking routines and to the
   user defined corruption handler routine, as long as these function pointers
   have been previously extablished by the initial call.  Note that we
   do this automatically when remapping a previously used heap, to ensure
   that the hooks get updated to the correct values, although the corruption
   handler pointer gets set back to the default.  The application can then
   call mmcheck to use a different corruption handler if desired.

   Returns non-zero if checking is successfully enabled, zero otherwise. */

int mmcheckf(void *md, void (*func) (void), int force)
{
  struct mdesc *mdp;
  int rtnval;

  mdp = MD_TO_MDP(md);

  /* We can safely set or update the abort function at any time, regardless
     of whether or not we successfully do anything else. */

  mdp->abortfunc = (func != NULL ? func : abort);

  /* If we haven't yet called mmalloc the first time for this heap, or if we
     have hooks that were previously installed, then allow the hooks to be
     initialized or updated. */

  if (force ||
      !(mdp->flags & MMALLOC_INITIALIZED) || (mdp->mfree_hook != NULL)) {
    mdp->mfree_hook = mfree_check;
    mdp->mmalloc_hook = mmalloc_check;
    mdp->mrealloc_hook = mrealloc_check;
    mdp->flags |= MMALLOC_MMCHECK_USED;
    rtnval = 1;
  } else {
    rtnval = 0;
  }

  return (rtnval);
}

/* This routine is for backwards compatibility only, in case there are
   still callers to the original mmcheck function. */

int mmcheck(void *md, void (*func) (void))
{
  int rtnval;

  rtnval = mmcheckf(md, func, 0);
  return (rtnval);
}
