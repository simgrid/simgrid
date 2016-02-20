/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURFXML_PARSE_H
#define _SURF_SURFXML_PARSE_H

#include <stdio.h>              /* to have FILE */
#include "xbt/misc.h"
#include "xbt/function_types.h"
#include "xbt/dict.h"

SG_BEGIN_DECL()
#include "surf/simgrid_dtd.h"

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

XBT_PUBLIC(void) surf_parse_open(const char *file);
XBT_PUBLIC(void) surf_parse_close(void);
XBT_PUBLIC(void) surf_parse_assert(bool cond, const char *fmt, ...) XBT_ATTRIB_PRINTF(2,3);
XBT_PUBLIC(void) XBT_ATTRIB_NORETURN surf_parse_error(const char *msg,...) XBT_ATTRIB_PRINTF(1,2);
XBT_PUBLIC(void) surf_parse_warn(const char *msg,...) XBT_ATTRIB_PRINTF(1,2);

XBT_PUBLIC(double) surf_parse_get_double(const char *string);
XBT_PUBLIC(int) surf_parse_get_int(const char *string);
XBT_PUBLIC(double) surf_parse_get_time(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_size(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_bandwidth(const char *string, const char *entity_kind, const char *name);
XBT_PUBLIC(double) surf_parse_get_speed(const char *string, const char *entity_kind, const char *name);

/* Prototypes of the functions offered by flex */
XBT_PUBLIC(int) surf_parse_lex(void);
XBT_PUBLIC(int) surf_parse_get_lineno(void);
XBT_PUBLIC(FILE *) surf_parse_get_in(void);
XBT_PUBLIC(FILE *) surf_parse_get_out(void);
XBT_PUBLIC(yy_size_t) surf_parse_get_leng(void);
XBT_PUBLIC(char *) surf_parse_get_text(void);
XBT_PUBLIC(void) surf_parse_set_lineno(int line_number);
XBT_PUBLIC(void) surf_parse_set_in(FILE * in_str);
XBT_PUBLIC(void) surf_parse_set_out(FILE * out_str);
XBT_PUBLIC(int) surf_parse_get_debug(void);
XBT_PUBLIC(void) surf_parse_set_debug(int bdebug);
XBT_PUBLIC(int) surf_parse_lex_destroy(void);

/* What is needed to bypass the parser. */
XBT_PUBLIC_DATA(int_f_void_t) surf_parse;       /* Entry-point to the parser. Set this to your function. */

SG_END_DECL()
#endif
