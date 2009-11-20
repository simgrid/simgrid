/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURFXML_PARSE_H
#define _SURF_SURFXML_PARSE_H

#include <stdio.h>              /* to have FILE */
#include "surf/simgrid_dtd.h"
#include "xbt/function_types.h"
#include "xbt/dict.h"


SG_BEGIN_DECL()

/* Hook for the different tags. All the functions which pointer to are push into here are run when the tag is encountered */
  XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_platform_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_platform_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_host_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_host_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_router_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_router_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_link_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_link_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_route_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_route_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_link_c_ctn_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_link_c_ctn_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_process_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_process_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_argument_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_argument_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_prop_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_prop_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_set_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_set_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_foreach_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_foreach_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_route_c_multi_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_route_c_multi_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_cluster_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_cluster_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_trace_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_trace_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_trace_c_connect_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_trace_c_connect_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) STag_surfxml_random_cb_list;
XBT_PUBLIC_DATA(xbt_dynar_t) ETag_surfxml_random_cb_list;

XBT_PUBLIC(void) surf_parse_open(const char *file);
XBT_PUBLIC(void) surf_parse_close(void);
XBT_PUBLIC(void) surf_parse_reset_parser(void);
XBT_PUBLIC(void) surf_parse_free_callbacks(void);
XBT_PUBLIC(void) surf_parse_get_double(double *value, const char *string);
XBT_PUBLIC(void) surf_parse_get_int(int *value, const char *string);

/* Prototypes of the functions offered by flex */
XBT_PUBLIC(int) surf_parse_lex(void);
XBT_PUBLIC(int) surf_parse_get_lineno(void);
XBT_PUBLIC(FILE *) surf_parse_get_in(void);
XBT_PUBLIC(FILE *) surf_parse_get_out(void);
XBT_PUBLIC(int) surf_parse_get_leng(void);
XBT_PUBLIC(char *) surf_parse_get_text(void);
XBT_PUBLIC(void) surf_parse_set_lineno(int line_number);
XBT_PUBLIC(void) surf_parse_set_in(FILE * in_str);
XBT_PUBLIC(void) surf_parse_set_out(FILE * out_str);
XBT_PUBLIC(int) surf_parse_get_debug(void);
XBT_PUBLIC(void) surf_parse_set_debug(int bdebug);
XBT_PUBLIC(int) surf_parse_lex_destroy(void);

/* What is needed to bypass the parser. */
XBT_PUBLIC_DATA(int_f_void_t) surf_parse;       /* Entry-point to the parser. Set this to your function. */

/* Set of macros to make the bypassing work easier.
 * See examples/msg/masterslave_bypass.c for an example of use */
#define SURFXML_BUFFER_SET(key,val) do { \
  AX_surfxml_##key=AX_ptr; \
  strcpy(A_surfxml_##key,val); \
  AX_ptr+=(int)strlen(val)+1; } while(0)

#define SURFXML_BUFFER_RESET() do { \
  AX_ptr = 0; \
  memset(surfxml_bufferstack,0,surfxml_bufferstack_size); } while(0)

#define SURFXML_START_TAG(tag)  STag_surfxml_##tag()
#define SURFXML_END_TAG(tag)  do { ETag_surfxml_##tag(); SURFXML_BUFFER_RESET(); } while(0)

XBT_PUBLIC(void) surfxml_add_callback(xbt_dynar_t cb_list,
                                      void_f_void_t function);


SG_END_DECL()
#endif
