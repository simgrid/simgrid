/* $Id$ */

/* xbt/error.h - Error tracking support                                     */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_ERROR_H
#define XBT_ERROR_H

#include <stdio.h> /* FIXME: Get rid of it */

#include "xbt/misc.h" /* BEGIN_DECL */
#include "xbt/log.h"

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>  /* to print the backtrace */
#endif

BEGIN_DECL

typedef enum {
  no_error=0,       /* succes */
  mismatch_error=1, /* The provided ID does not match */
  system_error=2,   /* a syscall did fail */
  network_error=3,  /* error while sending/receiving data */
  timeout_error=4,  /* not quick enough, dude */
  thread_error=5,   /* error while [un]locking */
  unknown_error=6,
     
  /* remote errors: result of a RMI/RPC.
     no_error(=0) is the same for both */   
  remote_mismatch_error=129,
  remote_system_error,
  remote_network_error,
  remote_timeout_error,
  remote_thread_error,
  remote_unknown_error
} xbt_error_t;

/*@observer@*/ const char *xbt_error_name(xbt_error_t errcode);

#define TRY(a) do {                                     \
  if ((errcode=a) != no_error) {                        \
     fprintf (stderr, "%s:%d: '%s' error raising...\n", \
	     __FILE__,__LINE__,                         \
             xbt_error_name(errcode));                 \
     return errcode;                                    \
  } } while (0)
   
#define TRYCATCH(a,b) if ((errcode=a) != no_error && errcode !=b) return errcode
#define TRYFAIL(a) do {                                        \
  if ((errcode=a) != no_error) {                               \
     fprintf(stderr,"%s:%d: Got '%s' error !\n",               \
	     __FILE__,__LINE__,                                \
             xbt_error_name(errcode));                        \
     fflush(stdout);                                           \
     xbt_abort();                                             \
  } } while(0)

#define TRYEXPECT(action,expected_error)  do {                 \
  errcode=action;                                              \
  if (errcode != expected_error) {                             \
    fprintf(stderr,"Got error %s (instead of %s expected)\n",  \
	    xbt_error_name(errcode),                          \
	    xbt_error_name(expected_error));                  \
    xbt_abort();                                              \
  }                                                            \
} while(0)

/* FIXME TRYCLEAN should be avoided for readability */
#define TRYCLEAN(action,cleanup) do {                          \
  if ((errcode=action) != no_error) {                          \
    cleanup;                                                   \
    return errcode;                                            \
  }                                                            \
} while(0)

#if 0 /* FIXME: We don't use backtrace. Drop it? */
#define _XBT_ERR_PRE do {                                     \
 void *_gs_array[30];                                          \
  size_t _gs_size= backtrace (_gs_array, 30);                  \
  char **_gs_strings= backtrace_symbols (_gs_array, _gs_size); \
  size_t _gs_i;

#define _XBT_ERR_POST(code)                                   \
  fprintf(stderr,"Backtrace follows\n");                       \
  for (_gs_i = 0; _gs_i < _gs_size; _gs_i++)                   \
     fprintf (stderr,"   %s\n", _gs_strings[_gs_i]);           \
  return code;                                                 \
} while (0)

#else /* if 0 */
#define _XBT_ERR_PRE do {
#define _XBT_ERR_POST(code)                                   \
  return code;                                                 \
} while (0)
#endif

#define RAISE0(code,fmt) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",    \
          __FILE__,__LINE__,__FUNCTION__); \
  _XBT_ERR_POST(code)
#define RAISE1(code,fmt,a1) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",       \
          __FILE__,__LINE__,__FUNCTION__,a1); \
  _XBT_ERR_POST(code)
#define RAISE2(code,fmt,a1,a2) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",          \
          __FILE__,__LINE__,__FUNCTION__,a1,a2); \
  _XBT_ERR_POST(code)
#define RAISE3(code,fmt,a1,a2,a3) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",             \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3); \
  _XBT_ERR_POST(code)
#define RAISE4(code,fmt,a1,a2,a3,a4) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",                \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3,a4); \
  _XBT_ERR_POST(code)
#define RAISE5(code,fmt,a1,a2,a3,a4,a5) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",                   \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3,a4,a5); \
  _XBT_ERR_POST(code)
#define RAISE6(code,fmt,a1,a2,a3,a4,a5,a6) _XBT_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",                      \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3,a4,a5,a6); \
  _XBT_ERR_POST(code)

/* #define RAISE_MALLOC     RAISE0(malloc_error,"Malloc error") */
#define RAISE_IMPOSSIBLE RAISE0(unknown_error,"The Impossible did happen")
#define RAISE_UNIMPLEMENTED RAISE1(unknown_error,"Function %s unimplemented",__FUNCTION__)

#ifdef NDEBUG
#define xbt_assert(cond)
#define xbt_assert0(cond,msg)
#define xbt_assert1(cond,msg,a)
#define xbt_assert2(cond,msg,a,b)
#define xbt_assert3(cond,msg,a,b,c)
#define xbt_assert4(cond,msg,a,b,c,d)
#define xbt_assert5(cond,msg,a,b,c,d,e)
#define xbt_assert6(cond,msg,a,b,c,d,e,f)
#else
#define xbt_assert(cond)                  if (!(cond)) { CRITICAL1("Assertion %s failed", #cond); xbt_abort(); }
#define xbt_assert0(cond,msg)             if (!(cond)) { CRITICAL0(msg); xbt_abort(); }
#define xbt_assert1(cond,msg,a)           if (!(cond)) { CRITICAL1(msg,a); xbt_abort(); }
#define xbt_assert2(cond,msg,a,b)         if (!(cond)) { CRITICAL2(msg,a,b); xbt_abort(); }
#define xbt_assert3(cond,msg,a,b,c)       if (!(cond)) { CRITICAL3(msg,a,b,c); xbt_abort(); }
#define xbt_assert4(cond,msg,a,b,c,d)     if (!(cond)) { CRITICAL4(msg,a,b,c,d); xbt_abort(); }
#define xbt_assert5(cond,msg,a,b,c,d,e)   if (!(cond)) { CRITICAL5(msg,a,b,c,d,e); xbt_abort(); }
#define xbt_assert6(cond,msg,a,b,c,d,e,f) if (!(cond)) { CRITICAL6(msg,a,b,c,d,e,f); xbt_abort(); }
#endif

void xbt_die(const char *msg) _XBT_GNUC_NORETURN;

#define DIE_IMPOSSIBLE xbt_assert0(0,"The Impossible did happen (yet again)")
#define xbt_assert_error(a) xbt_assert1(errcode == (a), "Error %s unexpected",xbt_error_name(errcode))

END_DECL

#endif /* XBT_ERROR_H */

