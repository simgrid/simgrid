/* $Id$ */

/* xbt.h - Public interface to the xbt (gras's toolbox)                   */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MISC_H
#define XBT_MISC_H

#define max(a, b) (((a) > (b))?(a):(b))
#define min(a, b) (((a) < (b))?(a):(b))

#define TRUE  1
#define FALSE 0

#define XBT_MAX_CHANNEL 10 /* FIXME: killme */
/*! C++ users need love */
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
/* Dunno where to place this: needed by config and amok */
typedef struct {  
   char *name;
   int port;
} xbt_host_t;

/* pointer to a function freeing something */
typedef   void (void_f_ppvoid_t)(void**);
typedef   void (void_f_pvoid_t) (void*);

/* The following two definitions concern the type of the keys used for
   the heaps. That should be handled via configure (FIXME). */
typedef long double xbt_heap_float_t;
#define XBT_HEAP_FLOAT_T "%Lg"	/* for printing purposes */

typedef long double xbt_maxmin_float_t;
#define XBT_MAXMIN_FLOAT_T "%Lg"	/* for printing purposes */


END_DECL

#endif /* XBT_MISC_H */
