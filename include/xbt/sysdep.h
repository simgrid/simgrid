/* $Id$ */

/* gras/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _GRAS_SYSDEP_H
#define _GRAS_SYSDEP_H

#include <string.h> /* Included directly for speed */

#include <time.h> /* FIXME: remove */
#include <unistd.h> /* FIXME: remove */

#ifdef  __cplusplus
extern "C" 
#endif

void* gras_malloc  (long int bytes);
void* gras_malloc0 (long int bytes);
void* gras_realloc (void  *memory, long int bytes);
void  gras_free    (void  *memory);

#define gras_new(type, count)  ((type*)gras_malloc (sizeof (type) * (count)))
#define gras_new0(type, count) ((type*)gras_malloc0 (sizeof (type) * (count)))

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define _GRAS_GNUC_PRINTF( format_idx, arg_idx )    \
	   __attribute__((__format__ (__printf__, format_idx, arg_idx)))
#define _GRAS_GNUC_SCANF( format_idx, arg_idx )     \
	       __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
#define _GRAS_GNUC_FORMAT( arg_idx )                \
		   __attribute__((__format_arg__ (arg_idx)))
#define _GRAS_GNUC_NORETURN                         \
     __attribute__((__noreturn__))
#else   /* !__GNUC__ */
#define _GRAS_GNUC_PRINTF( format_idx, arg_idx )
#define _GRAS_GNUC_SCANF( format_idx, arg_idx )
#define _GRAS_GNUC_FORMAT( arg_idx )
#define _GRAS_GNUC_NORETURN
#endif  /* !__GNUC__ */

void gras_abort(void) _GRAS_GNUC_NORETURN;

/* FIXME: This is a very good candidate to rewrite (along with a proper string stuff) 
   but I'm too lazy right now, so copy the definition */
long int strtol(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
int atoi(const char *nptr);


   
#ifdef  __cplusplus
}
#endif

#endif /* _GRAS_SYSDEP_H */
