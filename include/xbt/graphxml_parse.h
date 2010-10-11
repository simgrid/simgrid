/* 	$Id $	 */

/* Copyright (c) 2006, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPHXML_PARSE_H
#define _XBT_GRAPHXML_PARSE_H

#include <stdio.h>              /* to have FILE */
#include "xbt/misc.h"
#include "xbt/graphxml.h"
#include "simgrid_config.h"

/* Entry-point of the graphxml parser. */
extern int_f_void_t xbt_graph_parse;

/* Hook for the different tags. They can be redefined at will whereas
   the versions without the _fun can't. */
extern void_f_void_t STag_graphxml_graph_fun;
extern void_f_void_t ETag_graphxml_graph_fun;
extern void_f_void_t STag_graphxml_node_fun;
extern void_f_void_t ETag_graphxml_node_fun;
extern void_f_void_t STag_graphxml_edge_fun;
extern void_f_void_t ETag_graphxml_edge_fun;

XBT_PUBLIC(void) xbt_graph_parse_open(const char *file);
XBT_PUBLIC(void) xbt_graph_parse_close(void);
XBT_PUBLIC(void) xbt_graph_parse_reset_parser(void);
XBT_PUBLIC(void) xbt_graph_parse_get_double(double *value,
                                            const char *string);

/* Prototypes of the functions offered by flex */
XBT_PUBLIC(int) xbt_graph_parse_lex(void);
XBT_PUBLIC(int) xbt_graph_parse_get_lineno(void);
XBT_PUBLIC(FILE *) xbt_graph_parse_get_in(void);
XBT_PUBLIC(FILE *) xbt_graph_parse_get_out(void);
XBT_PUBLIC(int) xbt_graph_parse_get_leng(void);
XBT_PUBLIC(char *) xbt_graph_parse_get_text(void);
XBT_PUBLIC(void) xbt_graph_parse_set_lineno(int line_number);
XBT_PUBLIC(void) xbt_graph_parse_set_in(FILE * in_str);
XBT_PUBLIC(void) xbt_graph_parse_set_out(FILE * out_str);
XBT_PUBLIC(int) xbt_graph_parse_get_debug(void);
XBT_PUBLIC(void) xbt_graph_parse_set_debug(int bdebug);
XBT_PUBLIC(int) xbt_graph_parse_lex_destroy(void);

#endif
