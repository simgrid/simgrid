/* More debugging hooks for `mmalloc'.
   Copyright 1991, 1992, 1994 Free Software Foundation

   Written April 2, 1991 by John Gilmore of Cygnus Support
   Based on mcheck.c by Mike Haertel.
   Modified Mar 1992 by Fred Fish.  (fnf@cygnus.com) */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "mmprivate.h"

static void tr_break (void);
static void tr_freehook (void*md, void*ptr);
static void* tr_mallochook (void* md, size_t size);
static void* tr_reallochook (void* md, void* ptr, size_t size);

#ifndef	__GNU_LIBRARY__
extern char *getenv ();
#endif

static FILE *mallstream;

#if 0	/* FIXME:  Disabled for now. */
static char mallenv[] = "MALLOC_TRACE";
static char mallbuf[BUFSIZ];	/* Buffer for the output.  */
#endif

/* Address to breakpoint on accesses to... */
static void* mallwatch;

/* Old hook values.  */

static void (*old_mfree_hook) (void* md, void* ptr);
static void* (*old_mmalloc_hook) (void* md, size_t size);
static void* (*old_mrealloc_hook) (void* md, void* ptr, size_t size);

/* This function is called when the block being alloc'd, realloc'd, or
   freed has an address matching the variable "mallwatch".  In a debugger,
   set "mallwatch" to the address of interest, then put a breakpoint on
   tr_break.  */

static void
tr_break (void)
{
}

static void
tr_freehook (void *md, void *ptr)
{
  struct mdesc *mdp;

  mdp = MD_TO_MDP (md);
  /* Be sure to print it first.  */
  fprintf (mallstream, "- %08lx\n", (unsigned long) ptr);
  if (ptr == mallwatch)
    tr_break ();
  mdp -> mfree_hook = old_mfree_hook;
  mfree (md, ptr);
  mdp -> mfree_hook = tr_freehook;
}

static void*
tr_mallochook (void* md, size_t size)
{
  void* hdr;
  struct mdesc *mdp;

  mdp = MD_TO_MDP (md);
  mdp -> mmalloc_hook = old_mmalloc_hook;
  hdr = (void*) mmalloc (md, size);
  mdp -> mmalloc_hook = tr_mallochook;

  /* We could be printing a NULL here; that's OK.  */
  fprintf (mallstream, "+ %p 0x%lx\n", hdr, (unsigned long)size);

  if (hdr == mallwatch)
    tr_break ();

  return (hdr);
}

static void*
tr_reallochook (void *md, void *ptr, size_t size)
{
  void* hdr;
  struct mdesc *mdp;

  mdp = MD_TO_MDP (md);

  if (ptr == mallwatch)
    tr_break ();

  mdp -> mfree_hook = old_mfree_hook;
  mdp -> mmalloc_hook = old_mmalloc_hook;
  mdp -> mrealloc_hook = old_mrealloc_hook;
  hdr = (void*) mrealloc (md, ptr, size);
  mdp -> mfree_hook = tr_freehook;
  mdp -> mmalloc_hook = tr_mallochook;
  mdp -> mrealloc_hook = tr_reallochook;
  if (hdr == NULL)
    /* Failed realloc.  */
    fprintf (mallstream, "! %p 0x%lx\n", ptr, (unsigned long) size);
  else
    fprintf (mallstream, "< %p\n> %p 0x%lx\n", ptr,
	     hdr, (unsigned long) size);

  if (hdr == mallwatch)
    tr_break ();

  return hdr;
}

/* We enable tracing if either the environment variable MALLOC_TRACE
   is set, or if the variable mallwatch has been patched to an address
   that the debugging user wants us to stop on.  When patching mallwatch,
   don't forget to set a breakpoint on tr_break!  */

int
mmtrace (void)
{
#if 0	/* FIXME!  This is disabled for now until we figure out how to
	   maintain a stack of hooks per heap, since we might have other
	   hooks (such as set by mmcheck/mmcheckf) active also. */
  char *mallfile;

  mallfile = getenv (mallenv);
  if (mallfile  != NULL || mallwatch != NULL)
    {
      mallstream = fopen (mallfile != NULL ? mallfile : "/dev/null", "w");
      if (mallstream != NULL)
	{
	  /* Be sure it doesn't mmalloc its buffer!  */
	  setbuf (mallstream, mallbuf);
	  fprintf (mallstream, "= Start\n");
	  old_mfree_hook = mdp -> mfree_hook;
	  mdp -> mfree_hook = tr_freehook;
	  old_mmalloc_hook = mdp -> mmalloc_hook;
	  mdp -> mmalloc_hook = tr_mallochook;
	  old_mrealloc_hook = mdp -> mrealloc_hook;
	  mdp -> mrealloc_hook = tr_reallochook;
	}
    }

#endif	/* 0 */

  return (1);
}

