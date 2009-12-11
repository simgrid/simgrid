/* $Id$ */

/* str.h - XBT string related functions.                                    */

/* Copyright (c) 2004-7, Martin Quinson, Arnaud Legrand and  Cherier Malek. */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_STR_H
#define XBT_STR_H

#include <stdarg.h>             /* va_* */
#include "xbt/misc.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include <stdio.h>              /* FILE for getline */

SG_BEGIN_DECL()

/** @addtogroup XBT_str
 *  @brief String manipulation functions
 *
 * This module defines several string related functions. We redefine some quite classical
 * functions on the platforms were they are not nativaly defined (such as getline() or
 * asprintf()), while some other are a bit more exotic.
 * @{
 */


/* Trim related functions */
XBT_PUBLIC(void) xbt_str_rtrim(char *s, const char *char_list);
XBT_PUBLIC(void) xbt_str_ltrim(char *s, const char *char_list);
XBT_PUBLIC(void) xbt_str_trim(char *s, const char *char_list);

XBT_PUBLIC(xbt_dynar_t) xbt_str_split(const char *s, const char *sep);
XBT_PUBLIC(xbt_dynar_t) xbt_str_split_quoted(const char *s);
XBT_PUBLIC(xbt_dynar_t) xbt_str_split_str(const char *s, const char *sep);

XBT_PUBLIC(char *) xbt_str_join(xbt_dynar_t dynar, const char *sep);

/* */
XBT_PUBLIC(void) xbt_str_subst(char *str, char from, char to, int amount);
XBT_PUBLIC(char *) xbt_str_varsubst(char *str, xbt_dict_t patterns);

/* */
XBT_PUBLIC(void) xbt_str_strip_spaces(char *);
XBT_PUBLIC(char *) xbt_str_diff(char *a, char *b);


XBT_PUBLIC(char*)xbt_str_from_file(FILE *file);

/** @brief Classical alias to (char*)
 *
 * This of almost no use, beside cosmetics and the GRAS parsing macro (see \ref GRAS_dd_auto).
 */
     typedef char *xbt_string_t;

/**@}*/

SG_END_DECL()
#endif /* XBT_STR_H */
