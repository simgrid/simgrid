/* str.h - XBT string related functions.                                    */

/* Copyright (c) 2007-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_STR_H
#define XBT_STR_H

#include <stdarg.h>             /* va_* */
#include "xbt/misc.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "simgrid_config.h"     /* FILE for getline */

SG_BEGIN_DECL()

/** @addtogroup XBT_str
 *  @brief String manipulation functions
 *
 * This module defines several string related functions. We redefine some quite classical
 * functions on the platforms were they are not nativaly defined (such as xbt_getline() or
 * asprintf()), while some other are a bit more exotic.
 * @{
 */
/* Our own implementation of getline, mainly useful on the platforms not enjoying this function */
#include <stdio.h>  /* FILE */
#include <stdlib.h> /* size_t, ssize_t */
XBT_PUBLIC(ssize_t) xbt_getline(char **lineptr, size_t * n, FILE * stream);

/* Trim related functions */
XBT_PUBLIC(void) xbt_str_rtrim(char *s, const char *char_list);
XBT_PUBLIC(void) xbt_str_ltrim(char *s, const char *char_list);
XBT_PUBLIC(void) xbt_str_trim(char *s, const char *char_list);

XBT_PUBLIC(xbt_dynar_t) xbt_str_split(const char *s, const char *sep);
XBT_PUBLIC(xbt_dynar_t) xbt_str_split_quoted(const char *s);
XBT_PUBLIC(xbt_dynar_t) xbt_str_split_quoted_in_place(char *s);

XBT_PUBLIC(xbt_dynar_t) xbt_str_split_str(const char *s, const char *sep);

XBT_PUBLIC(char *) xbt_str_join(xbt_dynar_t dynar, const char *sep);
XBT_PUBLIC(char *) xbt_str_join_array(const char *const *strs, const char *sep);

/* */
XBT_PUBLIC(void) xbt_str_subst(char *str, char from, char to, int amount);
XBT_PUBLIC(char *) xbt_str_varsubst(const char *str, xbt_dict_t patterns);

/* */
XBT_PUBLIC(void) xbt_str_strip_spaces(char *);
XBT_PUBLIC(char *) xbt_str_diff(const char *a, const char *b);

XBT_PUBLIC(char *) xbt_str_from_file(FILE * file);

XBT_PUBLIC(int) xbt_str_start_with(const char* str, const char* start);

#define DJB2_HASH_FUNCTION
//#define FNV_HASH_FUNCTION

/**
 * @brief Returns the hash code of a string.
 */
static XBT_INLINE unsigned int xbt_str_hash_ext(const char *str, int str_len)
{

#ifdef DJB2_HASH_FUNCTION
  /* fast implementation of djb2 algorithm */
  int c;
  register unsigned int hash = 5381;

  while (str_len--) {
    c = *str++;
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }
# elif defined(FNV_HASH_FUNCTION)
  register unsigned int hash = 0x811c9dc5;
  unsigned char *bp = (unsigned char *) str;    /* start of buffer */
  unsigned char *be = bp + str_len;     /* beyond end of buffer */

  while (bp < be) {
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) +
        (hash << 24);

    /* xor the bottom with the current octet */
    hash ^= (unsigned int) *bp++;
  }

# else
  register unsigned int hash = 0;

  while (str_len--) {
    hash += (*str) * (*str);
    str++;
  }
#endif

  return hash;
}

/**
 * @brief Returns the hash code of a string.
 */
static XBT_INLINE unsigned int xbt_str_hash(const char *str)
{
#ifdef DJB2_HASH_FUNCTION
  /* fast implementation of djb2 algorithm */
  int c;
  register unsigned int hash = 5381;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }

# elif defined(FNV_HASH_FUNCTION)
  register unsigned int hash = 0x811c9dc5;

  while (*str) {
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hash +=
        (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) +
        (hash << 24);

    /* xor the bottom with the current byte */
    hash ^= (unsigned int) *str++;
  }

# else
  register unsigned int hash = 0;

  while (*str) {
    hash += (*str) * (*str);
    str++;
  }
#endif
  return hash;
}
                                                      
/**@}*/

SG_END_DECL()
#endif                          /* XBT_STR_H */
