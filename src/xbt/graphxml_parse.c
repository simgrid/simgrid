/* 	$Id$	 */

/* Copyright (c) 2006 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"

#include "xbt/dynar.h"
#include "xbt/graphxml_parse.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(graphxml_parse, xbt,
                                "Logging specific to the graphxml parsing  module");

#undef CLEANUP
#include "graphxml.c"

static xbt_dynar_t xbt_graph_input_buffer_stack = NULL;
static xbt_dynar_t xbt_graph_file_to_parse_stack = NULL;

static void nil_function(void)
{
  return;
}

void_f_void_t STag_graphxml_graph_fun = nil_function;
void_f_void_t ETag_graphxml_graph_fun = nil_function;
void_f_void_t STag_graphxml_node_fun = nil_function;
void_f_void_t ETag_graphxml_node_fun = nil_function;
void_f_void_t STag_graphxml_edge_fun = nil_function;
void_f_void_t ETag_graphxml_edge_fun = nil_function;

YY_BUFFER_STATE xbt_graph_input_buffer;
FILE *xbt_graph_file_to_parse;

void xbt_graph_parse_reset_parser(void)
{
  STag_graphxml_graph_fun = nil_function;
  ETag_graphxml_graph_fun = nil_function;
  STag_graphxml_node_fun = nil_function;
  ETag_graphxml_node_fun = nil_function;
  STag_graphxml_edge_fun = nil_function;
  ETag_graphxml_edge_fun = nil_function;
}

void STag_graphxml_graph(void)
{
  (*STag_graphxml_graph_fun) ();
}

void ETag_graphxml_graph(void)
{
  (*ETag_graphxml_graph_fun) ();
}


void STag_graphxml_node(void)
{
  (*STag_graphxml_node_fun) ();
}

void ETag_graphxml_node(void)
{
  (*ETag_graphxml_node_fun) ();
}


void STag_graphxml_edge(void)
{
  (*STag_graphxml_edge_fun) ();
}

void ETag_graphxml_edge(void)
{
  (*ETag_graphxml_edge_fun) ();
}



void xbt_graph_parse_open(const char *file)
{
  if (!file) {
    WARN0
      ("I hope you know what you're doing... you just gave me a NULL pointer!");
    return;
  }
  if (!xbt_graph_input_buffer_stack)
    xbt_graph_input_buffer_stack =
      xbt_dynar_new(sizeof(YY_BUFFER_STATE), NULL);
  if (!xbt_graph_file_to_parse_stack)
    xbt_graph_file_to_parse_stack = xbt_dynar_new(sizeof(FILE *), NULL);

  xbt_graph_file_to_parse = fopen(file, "r");   /* FIXME should use something like surf_fopen */
  xbt_assert1((xbt_graph_file_to_parse), "Unable to open \"%s\"\n", file);
  xbt_graph_input_buffer =
    xbt_graph_parse__create_buffer(xbt_graph_file_to_parse, 10);
  xbt_graph_parse__switch_to_buffer(xbt_graph_input_buffer);
  xbt_graph_parse_lineno = 1;
}

void xbt_graph_parse_close(void)
{
  if (xbt_graph_input_buffer_stack)
    xbt_dynar_free(&xbt_graph_input_buffer_stack);
  if (xbt_graph_file_to_parse_stack)
    xbt_dynar_free(&xbt_graph_file_to_parse_stack);

  if (xbt_graph_file_to_parse) {
    xbt_graph_parse__delete_buffer(xbt_graph_input_buffer);
    fclose(xbt_graph_file_to_parse);
  }
}


static int _xbt_graph_parse(void)
{
  return xbt_graph_parse_lex();
}

int_f_void_t xbt_graph_parse = _xbt_graph_parse;

void xbt_graph_parse_get_double(double *value, const char *string)
{
  int ret = 0;

  ret = sscanf(string, "%lg", value);
  xbt_assert2((ret == 1), "Parse error line %d : %s not a number",
              xbt_graph_parse_lineno, string);
}
