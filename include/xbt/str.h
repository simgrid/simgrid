/* $Id$ */

/* str.h - XBT string related functions.                                    */

/* Copyright (c) 2004-7, Martin Quinson and Arnaud Legrand.                 */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"



#ifndef XBT_STR_H
#define XBT_STR_H

#if defined(_WIN32)
#include <stdio.h>
#endif

SG_BEGIN_DECL()

XBT_PUBLIC(int) asprintf  (char **ptr, const char *fmt, /*args*/ ...) _XBT_GNUC_PRINTF(2,3);
XBT_PUBLIC(int) vasprintf (char **ptr, const char *fmt, va_list ap);
XBT_PUBLIC(char*) bprintf   (const char*fmt, ...) _XBT_GNUC_PRINTF(1,2);

#if defined(_WIN32) || !defined(__GNUC__)
XBT_PUBLIC(long) getline(char **lineptr, size_t *n, FILE *stream);
#endif

SG_END_DECL()

#endif /* XBT_STR_H */




