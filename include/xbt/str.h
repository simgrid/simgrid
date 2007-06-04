/* $Id$ */

/* str.h - XBT string related functions.                                    */

/* Copyright (c) 2004-7, Martin Quinson, Arnaud Legrand and  Cherier Malek. */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/dynar.h"

#ifndef XBT_STR_H
#define XBT_STR_H

#if defined(_WIN32)
#include <stdio.h>
#endif

SG_BEGIN_DECL()

/** @addtogroup XBT_str
 *  @brief String manipulation functions
 * 
 * This module defines several string related functions. We redefine some quite classical 
 * functions on the platforms were they are not nativaly defined (such as getline() or 
 * asprintf()), while some other are a bit more exotic. 
 * @{
 */
  
/* snprintf related functions */
/** @brief print to allocated string (reimplemented when not provided by the system)
 * 
 * The functions asprintf() and vasprintf() are analogues of
 * sprintf() and vsprintf(), except that they allocate a string large
 * enough to hold the output including the terminating null byte, and
 * return a pointer to it via the first parameter.  This pointer
 * should be passed to free(3) to release the allocated storage when
 * it is no longer needed.
 */
XBT_PUBLIC(int) asprintf  (char **ptr, const char *fmt, /*args*/ ...) _XBT_GNUC_PRINTF(2,3);
/** @brief print to allocated string (reimplemented when not provided by the system)
 * 
 * See asprintf()
 */
XBT_PUBLIC(int) vasprintf (char **ptr, const char *fmt, va_list ap);
/** @brief print to allocated string
 * 
 * Works just like asprintf(), but returns a pointer to the newly created string
 */
XBT_PUBLIC(char*) bprintf   (const char*fmt, ...) _XBT_GNUC_PRINTF(1,2);

/* the gettext function. It gets redefined here only if not yet available */
#if defined(_WIN32) || !defined(__GNUC__) || defined(DOXYGEN)
XBT_PUBLIC(long) getline(char **lineptr, size_t *n, FILE *stream);
#endif

/* Trim related functions */
XBT_PUBLIC(void) xbt_str_rtrim(char* s, const char* char_list);
XBT_PUBLIC(void) xbt_str_ltrim( char* s, const char* char_list);
XBT_PUBLIC(void) xbt_str_trim(char* s, const char* char_list);

XBT_PUBLIC(xbt_dynar_t) xbt_str_split(const char *s, const char *sep);
XBT_PUBLIC(char *) xbt_str_join(xbt_dynar_t dynar, const char *sep);

/* */
XBT_PUBLIC(void) xbt_str_strip_spaces(char *);
XBT_PUBLIC(char *) xbt_str_diff(char *a, char *b);

/**@}*/

SG_END_DECL()

#endif /* XBT_STR_H */




