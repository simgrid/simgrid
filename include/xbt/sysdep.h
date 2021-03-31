/*  xbt/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_SYSDEP_H
#define XBT_SYSDEP_H

#include <xbt/asserts.h>
#include <xbt/log.h>

#include <simgrid/config.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>             /* va_list */

SG_BEGIN_DECL

#ifdef XBT_LOG_LOCALLY_DEFINE_XBT_CHANNEL
XBT_LOG_NEW_CATEGORY(xbt, "All XBT categories (simgrid toolbox)");
XBT_LOG_NEW_SUBCATEGORY(xbt_help, xbt, "Help messages");
#else
XBT_LOG_EXTERNAL_CATEGORY(xbt);
XBT_LOG_EXTERNAL_CATEGORY(xbt_help);
#endif

/** @addtogroup XBT_syscall
 *  @brief Malloc and associated functions, killing the program on error (with @ref XBT_ex)
 *
 *  @{
 */

/** @brief Like strdup, but xbt_die() on error */
static XBT_ALWAYS_INLINE char *xbt_strdup(const char *s) {
  char *res = NULL;
  if (s) {
    res = strdup(s);
    if (!res)
      xbt_die("memory allocation error (strdup returned NULL)");
  }
  return res;
}

/** @brief Like malloc, but xbt_die() on error
    @hideinitializer */
static XBT_ALWAYS_INLINE void *xbt_malloc(size_t n) {
  void* res = malloc(n);
  if (!res)
    xbt_die("Memory allocation of %lu bytes failed", (unsigned long)n);
  return res;
}

/** @brief like malloc, but xbt_die() on error and memset data to 0
    @hideinitializer */
static XBT_ALWAYS_INLINE void *xbt_malloc0(size_t n) {
  void* res = calloc(n, 1);
  if (!res)
    xbt_die("Memory callocation of %lu bytes failed", (unsigned long)n);
  return res;
}

/** @brief like realloc, but xbt_die() on error
    @hideinitializer */
static XBT_ALWAYS_INLINE void *xbt_realloc(void *p, size_t s) {
  void *res = NULL;
  if (s) {
    if (p) {
      res = realloc(p, s);
      if (!res)
        xbt_die("memory (re)allocation of %lu bytes failed", (unsigned long)s);
    } else {
      res = xbt_malloc(s);
    }
  } else {
    free(p);
  }
  return res;
}

/** @brief like free
    @hideinitializer */
#define xbt_free(p) free(p) /*nothing specific to do here. A poor valgrind replacement? */

#ifdef __cplusplus
#define XBT_FREE_NOEXCEPT noexcept(noexcept(::free))
#else
#define XBT_FREE_NOEXCEPT
#endif

/** @brief like free, but you can be sure that it is a function  */
XBT_PUBLIC void xbt_free_f(void* p) XBT_FREE_NOEXCEPT;
/** @brief should be given a pointer to pointer, and frees the second one */
XBT_PUBLIC void xbt_free_ref(void* d) XBT_FREE_NOEXCEPT;

SG_END_DECL

#define xbt_new(type, count)  ((type*)xbt_malloc (sizeof (type) * (count)))
/** @brief like calloc, but xbt_die() on error
    @hideinitializer */
#define xbt_new0(type, count) ((type*)xbt_malloc0 (sizeof (type) * (count)))

/** @} */

#endif                          /* XBT_SYSDEP_H */
