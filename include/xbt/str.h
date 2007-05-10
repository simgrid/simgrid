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

/* snprintf related functions */
XBT_PUBLIC(int) asprintf  (char **ptr, const char *fmt, /*args*/ ...) _XBT_GNUC_PRINTF(2,3);
XBT_PUBLIC(int) vasprintf (char **ptr, const char *fmt, va_list ap);
XBT_PUBLIC(char*) bprintf   (const char*fmt, ...) _XBT_GNUC_PRINTF(1,2);

/* the gettext function. It gets redefined here only if not yet available */
#if defined(_WIN32) || !defined(__GNUC__)
XBT_PUBLIC(long) getline(char **lineptr, size_t *n, FILE *stream);
#endif

/* Trim related functions */
XBT_PUBLIC(void) xbt_str_rtrim(char* s, const char* char_list);
XBT_PUBLIC(void) xbt_str_ltrim( char* s, const char* char_list);
XBT_PUBLIC(void) xbt_str_trim(char* s, const char* char_list);

XBT_PUBLIC(xbt_dynar_t) xbt_str_split(char *s, const char *sep);
XBT_PUBLIC(char *) xbt_str_join(xbt_dynar_t dynar, const char *sep);

/* */
XBT_PUBLIC(void) xbt_str_strip_spaces(char *);
XBT_PUBLIC(char *) xbt_str_diff(char *a, char *b);

SG_END_DECL()

#endif /* XBT_STR_H */




