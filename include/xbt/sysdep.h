/* $Id$ */
/*  xbt/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_SYSDEP_H
#define _XBT_SYSDEP_H

#include <string.h>
#include <stdlib.h> 
   
#include "xbt/misc.h"
#include "xbt/error.h"
  
BEGIN_DECL()
/** @addtogroup XBT_syscall
 *  @{
 */

/** @brief like strdup, but xbt_die() on error */
static __inline__ char *xbt_strdup(const char *s) {
  char *res = NULL;
  if (s) {
    res=strdup(s);
    if (!res) 
      xbt_die("memory allocation error");
  } 
  return res;
}
/** @brief like malloc, but xbt_die() on error 
    @hideinitializer */
static __inline__ void *xbt_malloc(int n){
  void *res=malloc(n);
  if (!res)
     xbt_die("Memory allocation failed");
  return res;
}

/** @brief like malloc, but xbt_die() on error and memset data to 0
    @hideinitializer */
static __inline__ void *xbt_malloc0(int n) {
  void *res=calloc(n,1);
  if (!res)
     xbt_die("Memory callocation failed");
  return res;
}
  
/** @brief like realloc, but xbt_die() on error 
    @hideinitializer */
static __inline__ void *xbt_realloc(void*p,int s){
  void *res=res;
  if (s) {
    if (p) {
      res=realloc(p,s);
      if (!res) 
	xbt_die("memory allocation error");
    } else {
      res=xbt_malloc(s);
    }
  } else {
    if (p) {
      free(p);
    }
  }
  return res;
}
/** @brief like free
    @hideinitializer */
#define xbt_free free /*nothing specific to do here. A poor valgrind replacement?*/
/*#define xbt_free_fct free * replacement with the guareenty of being a function  FIXME:KILLME*/
   
/** @brief like calloc, but xbt_die() on error and don't memset to 0
    @hideinitializer */
#define xbt_new(type, count)  ((type*)xbt_malloc (sizeof (type) * (count)))
/** @brief like calloc, but xbt_die() on error
    @hideinitializer */
#define xbt_new0(type, count) ((type*)xbt_malloc0 (sizeof (type) * (count)))

/** @} */  

END_DECL()

#endif /* _XBT_SYSDEP_H */
