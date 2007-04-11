/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURFXML_PARSE_H
#define _SURF_SURFXML_PARSE_H

#include <stdio.h> /* to have FILE */
#include "surf/surfxml.h"
#include "xbt/function_types.h"
/* Entry-point of the surfxml parser. */
extern int_f_void_t * XBT_PUBLIC_DATA surf_parse;

/* Hook for the different tags. They can be redefined at will whereas
   the versions without the _fun can't. */

extern void_f_void_t *STag_surfxml_platform_description_fun;
extern void_f_void_t *ETag_surfxml_platform_description_fun;
extern void_f_void_t *STag_surfxml_cpu_fun;
extern void_f_void_t *ETag_surfxml_cpu_fun;
extern void_f_void_t *STag_surfxml_network_link_fun;
extern void_f_void_t *ETag_surfxml_network_link_fun;
extern void_f_void_t *STag_surfxml_route_fun;
extern void_f_void_t *ETag_surfxml_route_fun;
extern void_f_void_t *STag_surfxml_route_element_fun;
extern void_f_void_t *ETag_surfxml_route_element_fun;
extern void_f_void_t * XBT_PUBLIC_DATA STag_surfxml_process_fun;
extern void_f_void_t * XBT_PUBLIC_DATA ETag_surfxml_process_fun;
extern void_f_void_t *STag_surfxml_argument_fun;
extern void_f_void_t * XBT_PUBLIC_DATA ETag_surfxml_argument_fun;

XBT_PUBLIC(void) surf_parse_open(const char *file);
XBT_PUBLIC(void) surf_parse_close(void);
XBT_PUBLIC(void) surf_parse_reset_parser(void);
XBT_PUBLIC(void) surf_parse_get_double(double *value,const char *string);

/* Prototypes of the functions offered by flex */
XBT_PUBLIC(int) surf_parse_lex(void);
XBT_PUBLIC(int) surf_parse_get_lineno(void);
XBT_PUBLIC(FILE*) surf_parse_get_in(void);
XBT_PUBLIC(FILE*) surf_parse_get_out(void);
XBT_PUBLIC(int) surf_parse_get_leng(void);
XBT_PUBLIC(char*) surf_parse_get_text(void);
XBT_PUBLIC(void) surf_parse_set_lineno(int line_number);
XBT_PUBLIC(void) surf_parse_set_in(FILE * in_str);
XBT_PUBLIC(void) surf_parse_set_out(FILE * out_str);
XBT_PUBLIC(int) surf_parse_get_debug(void);
XBT_PUBLIC(void) surf_parse_set_debug(int bdebug);
XBT_PUBLIC(int) surf_parse_lex_destroy(void);

#endif
