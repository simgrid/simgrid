/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_PARSE_H
#define _SURF_SURF_PARSE_H

#include "xbt/misc.h"
#include "surf/trace_mgr.h"
#include "surfxml.h"

typedef   void (*void_f_void_t)(void);

extern void_f_void_t STag_platform_description_fun;
extern void_f_void_t ETag_platform_description_fun;
extern void_f_void_t STag_cpu_fun;
extern void_f_void_t ETag_cpu_fun;
extern void_f_void_t STag_network_link_fun;
extern void_f_void_t ETag_network_link_fun;
extern void_f_void_t STag_route_fun;
extern void_f_void_t ETag_route_fun;
extern void_f_void_t STag_route_element_fun;
extern void_f_void_t ETag_route_element_fun;
extern void_f_void_t STag_process_fun;
extern void_f_void_t ETag_process_fun;
extern void_f_void_t STag_argument_fun;
extern void_f_void_t ETag_argument_fun;


void  surf_parse_open(const char *file);
void  surf_parse_close(void);
void  surf_parse_reset_parser(void);
void surf_parse_get_double(double *value,const char *string);
void surf_parse_get_trace(tmgr_trace_t *trace, const char *string);

/* Prototypes of the functions offered by flex */
int surf_parse_lex(void);
int surf_parse_get_lineno(void);
FILE *surf_parse_get_in(void);
FILE *surf_parse_get_out(void);
int surf_parse_get_leng(void);
char *surf_parse_get_text(void);
void surf_parse_set_lineno(int line_number);
void surf_parse_set_in(FILE * in_str);
void surf_parse_set_out(FILE * out_str);
int surf_parse_get_debug(void);
void surf_parse_set_debug(int bdebug);
int surf_parse_lex_destroy(void);

#endif
