/* str.h - XBT string related functions.                                    */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_STR_H
#define XBT_STR_H

#include <xbt/dict.h>
#include <xbt/dynar.h>
#include <xbt/misc.h>

#include <stdarg.h> /* va_* */

SG_BEGIN_DECL

/** @addtogroup XBT_str
 *  @brief String manipulation functions
 *
 * This module defines several string related functions. Looking at the diversity of string manipulation functions that
 * are provided, you can see that several SimGrid core developers actually like Perl.
 * @{
 */

XBT_PUBLIC xbt_dynar_t xbt_str_split_quoted(const char* s);
XBT_PUBLIC xbt_dynar_t xbt_str_split_quoted_in_place(char* s);

XBT_PUBLIC long int xbt_str_parse_int(const char* str, const char* error_mesg);
XBT_PUBLIC double xbt_str_parse_double(const char* str, const char* error_mesg);

#define XBT_DJB2_HASH_FUNCTION
//#define XBT_FNV_HASH_FUNCTION

/**
 * @brief Returns the hash code of a string.
 */
static inline unsigned int xbt_str_hash_ext(const char* str, int str_len)
{
#ifdef XBT_DJB2_HASH_FUNCTION
  /* fast implementation of djb2 algorithm */
  unsigned int hash = 5381;

  while (str_len--) {
    int c = *str++;
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }
# elif defined(XBT_FNV_HASH_FUNCTION)
  unsigned int hash = 0x811c9dc5;
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
  unsigned int hash = 0;

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
static inline unsigned int xbt_str_hash(const char *str)
{
#ifdef XBT_DJB2_HASH_FUNCTION
  /* fast implementation of djb2 algorithm */
  int c;
  unsigned int hash = 5381;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;    /* hash * 33 + c */
  }

# elif defined(XBT_FNV_HASH_FUNCTION)
  unsigned int hash = 0x811c9dc5;

  while (*str) {
    /* multiply by the 32 bit FNV magic prime mod 2^32 */
    hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);

    /* xor the bottom with the current byte */
    hash ^= (unsigned int) *str++;
  }

# else
  unsigned int hash = 0;

  while (*str) {
    hash += (*str) * (*str);
    str++;
  }
#endif
  return hash;
}

/**@}*/
SG_END_DECL
#endif                          /* XBT_STR_H */
