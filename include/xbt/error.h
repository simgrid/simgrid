/* $Id$ */

/* gras/error.h - Error tracking support                                    */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef GRAS_ERROR_H
#define GRAS_ERROR_H

#include <stddef.h>    /* offsetof() */
#include <sys/types.h>  /* size_t */
#include <stdarg.h>

#include <stdio.h> /* FIXME: Get rid of it */

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>  /* to print the backtrace */
#endif

/* C++ users need love */
#ifndef BEGIN_DECL
# ifdef __cplusplus
#  define BEGIN_DECL extern "C" {
# else
#  define BEGIN_DECL 
# endif
#endif

/*! C++ users need love */
#ifndef END_DECL
# ifdef __cplusplus
#  define END_DECL }
# else
#  define END_DECL 
# endif
#endif
/* End of cruft for C++ */

BEGIN_DECL

typedef enum {
  no_error=0,     /* succes */
  malloc_error,   /* Well known error */
  mismatch_error, /* The provided ID does not match */
  system_error,   /* a syscall did fail */
  network_error,  /* error while sending/receiving data */
  timeout_error,  /* not quick enough, dude */
  thread_error,   /* error while [un]locking */
  unknown_error 
} gras_error_t;

/*@observer@*/ const char *gras_error_name(gras_error_t errcode);

#define TRY(a) do {                                     \
  if ((errcode=a) != no_error) {                        \
     fprintf (stderr, "%s:%d: '%s' error raising...\n", \
	     __FILE__,__LINE__,                         \
             gras_error_name(errcode));                 \
     return errcode;                                    \
  } } while (0)
   
#define TRYCATCH(a,b) if ((errcode=a) != no_error && errcode !=b) return errcode
#define TRYFAIL(a) do {                                        \
  if ((errcode=a) != no_error) {                               \
     fprintf(stderr,"%s:%d: Got '%s' error !\n",               \
	     __FILE__,__LINE__,                                \
             gras_error_name(errcode));                        \
     fflush(stdout);                                           \
     gras_abort();                                             \
  } } while(0)

#define TRYEXPECT(action,expected_error)  do {                 \
  errcode=action;                                              \
  if (errcode != expected_error) {                             \
    fprintf(stderr,"Got error %s (instead of %s expected)\n",  \
	    gras_error_name(errcode),                          \
	    gras_error_name(expected_error));                  \
    gras_abort();                                              \
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
#define _GRAS_ERR_PRE do {                                     \
 void *_gs_array[30];                                          \
  size_t _gs_size= backtrace (_gs_array, 30);                  \
  char **_gs_strings= backtrace_symbols (_gs_array, _gs_size); \
  size_t _gs_i;

#define _GRAS_ERR_POST(code)                                   \
  fprintf(stderr,"Backtrace follows\n");                       \
  for (_gs_i = 0; _gs_i < _gs_size; _gs_i++)                   \
     fprintf (stderr,"   %s\n", _gs_strings[_gs_i]);           \
  return code;                                                 \
} while (0)

#else
#define _GRAS_ERR_PRE do {
#define _GRAS_ERR_POST(code)                                   \
  return code;                                                 \
} while (0)
#endif

#define RAISE0(code,fmt) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",    \
          __FILE__,__LINE__,__FUNCTION__); \
  _GRAS_ERR_POST(code)
#define RAISE1(code,fmt,a1) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",       \
          __FILE__,__LINE__,__FUNCTION__,a1); \
  _GRAS_ERR_POST(code)
#define RAISE2(code,fmt,a1,a2) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",          \
          __FILE__,__LINE__,__FUNCTION__,a1,a2); \
  _GRAS_ERR_POST(code)
#define RAISE3(code,fmt,a1,a2,a3) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",             \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3); \
  _GRAS_ERR_POST(code)
#define RAISE4(code,fmt,a1,a2,a3,a4) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",                \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3,a4); \
  _GRAS_ERR_POST(code)
#define RAISE5(code,fmt,a1,a2,a3,a4,a5) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",                   \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3,a4,a5); \
  _GRAS_ERR_POST(code)
#define RAISE6(code,fmt,a1,a2,a3,a4,a5,a6) _GRAS_ERR_PRE     \
  fprintf(stderr,"%s:%d:%s: " fmt "\n",                      \
          __FILE__,__LINE__,__FUNCTION__,a1,a2,a3,a4,a5,a6); \
  _GRAS_ERR_POST(code)

#define RAISE_MALLOC     RAISE0(malloc_error,"Malloc error")
#define RAISE_IMPOSSIBLE RAISE0(unknown_error,"The Impossible did happen")
#define RAISE_UNIMPLEMENTED RAISE1(unknown_error,"Function %s unimplemented",__FUNCTION__)

#ifdef NDEBUG
#define gras_assert(cond)
#define gras_assert0(cond,msg)
#define gras_assert1(cond,msg,a)
#define gras_assert2(cond,msg,a,b)
#define gras_assert3(cond,msg,a,b,c)
#define gras_assert4(cond,msg,a,b,c,d)
#define gras_assert5(cond,msg,a,b,c,d,e)
#define gras_assert6(cond,msg,a,b,c,d,e,f)
#else
#define gras_assert(cond)                  if (!(cond)) { CRITICAL1("Assertion %s failed", #cond); gras_abort(); }
#define gras_assert0(cond,msg)             if (!(cond)) { CRITICAL0(msg); gras_abort(); }
#define gras_assert1(cond,msg,a)           if (!(cond)) { CRITICAL1(msg,a); gras_abort(); }
#define gras_assert2(cond,msg,a,b)         if (!(cond)) { CRITICAL2(msg,a,b); gras_abort(); }
#define gras_assert3(cond,msg,a,b,c)       if (!(cond)) { CRITICAL3(msg,a,b,c); gras_abort(); }
#define gras_assert4(cond,msg,a,b,c,d)     if (!(cond)) { CRITICAL4(msg,a,b,c,d); gras_abort(); }
#define gras_assert5(cond,msg,a,b,c,d,e)   if (!(cond)) { CRITICAL5(msg,a,b,c,d,e); gras_abort(); }
#define gras_assert6(cond,msg,a,b,c,d,e,f) if (!(cond)) { CRITICAL6(msg,a,b,c,d,e,f); gras_abort(); }
#endif
END_DECL

#endif /* GRAS_ERROR_H */

