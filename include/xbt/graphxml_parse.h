/* 	$Id $	 */

/* Copyright (c) 2006 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_GRAPHXML_PARSE_H
#define _XBT_GRAPHXML_PARSE_H

#include <stdio.h>
#include "xbt/misc.h"
#include "xbt/graphxml.h"

/* Entry-point of the graphxml parser. */
extern int_f_void_t *xbt_graph_parse;

/* Hook for the different tags. They can be redefined at will whereas
   the versions without the _fun can't. */
extern void_f_void_t *STag_graphxml_graph_fun;
extern void_f_void_t *ETag_graphxml_graph_fun;
extern void_f_void_t *STag_graphxml_node_fun;
extern void_f_void_t *ETag_graphxml_node_fun;
extern void_f_void_t *STag_graphxml_edge_fun;
extern void_f_void_t *ETag_graphxml_edge_fun;

void xbt_graph_parse_open(const char *file);
void xbt_graph_parse_close(void);
void xbt_graph_parse_reset_parser(void);
void xbt_graph_parse_get_double(double *value,const char *string);

/* Prototypes of the functions offered by flex */
int xbt_graph_parse_lex(void);
int xbt_graph_parse_get_lineno(void);
FILE *xbt_graph_parse_get_in(void);
FILE *xbt_graph_parse_get_out(void);
int xbt_graph_parse_get_leng(void);
char *xbt_graph_parse_get_text(void);
void xbt_graph_parse_set_lineno(int line_number);
void xbt_graph_parse_set_in(FILE * in_str);
void xbt_graph_parse_set_out(FILE * out_str);
int xbt_graph_parse_get_debug(void);
void xbt_graph_parse_set_debug(int bdebug);
int xbt_graph_parse_lex_destroy(void);

#endif
