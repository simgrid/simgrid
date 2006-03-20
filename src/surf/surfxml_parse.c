/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "surf/surfxml_parse_private.h"
#include "surf/surf_private.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(parse, surf ,"Logging specific to the SURF  module");

#undef CLEANUP
#include "surfxml.c"

static xbt_dynar_t surf_input_buffer_stack=NULL;
static xbt_dynar_t surf_file_to_parse_stack=NULL;

void nil_function(void);
void nil_function(void)
{
  return;
}

void_f_void_t STag_platform_description_fun = nil_function;
void_f_void_t ETag_platform_description_fun = nil_function;
void_f_void_t STag_cpu_fun = nil_function;
void_f_void_t ETag_cpu_fun = nil_function;
void_f_void_t STag_network_link_fun = nil_function;
void_f_void_t ETag_network_link_fun = nil_function;
void_f_void_t STag_route_fun = nil_function;
void_f_void_t ETag_route_fun = nil_function;
void_f_void_t STag_route_element_fun = nil_function;
void_f_void_t ETag_route_element_fun = nil_function;
void_f_void_t STag_process_fun = nil_function;
void_f_void_t ETag_process_fun = nil_function;
void_f_void_t STag_argument_fun = nil_function;
void_f_void_t ETag_argument_fun = nil_function;

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse;

void surf_parse_reset_parser(void)
{
  STag_platform_description_fun = nil_function;
  ETag_platform_description_fun = nil_function;
  STag_cpu_fun = nil_function;
  ETag_cpu_fun = nil_function;
  STag_network_link_fun = nil_function;
  ETag_network_link_fun = nil_function;
  STag_route_fun = nil_function;
  ETag_route_fun = nil_function;
  STag_route_element_fun = nil_function;
  ETag_route_element_fun = nil_function;
  STag_process_fun = nil_function;
  ETag_process_fun = nil_function;
  STag_argument_fun = nil_function;
  ETag_argument_fun = nil_function;
}

void STag_include(void)
{
  xbt_dynar_push(surf_input_buffer_stack,&surf_input_buffer);
  xbt_dynar_push(surf_file_to_parse_stack,&surf_file_to_parse);
  
  surf_file_to_parse = surf_fopen(A_include_file,"r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n",A_include_file);
  surf_input_buffer = surf_parse__create_buffer( surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  printf("STAG\n"); fflush(NULL);
}

void ETag_include(void)
{
  printf("ETAG\n"); fflush(NULL);
  surf_parse__delete_buffer(surf_input_buffer);
  fclose(surf_file_to_parse);
  xbt_dynar_pop(surf_file_to_parse_stack,&surf_file_to_parse);
  xbt_dynar_pop(surf_input_buffer_stack,&surf_input_buffer);
}

void STag_platform_description(void)
{
  STag_platform_description_fun();
}

void ETag_platform_description(void)
{
  ETag_platform_description_fun();
}

void STag_cpu(void)
{
  STag_cpu_fun();
}

void ETag_cpu(void)
{
  ETag_cpu_fun();
}

void STag_network_link(void)
{
  STag_network_link_fun();
}

void ETag_network_link(void)
{
  ETag_network_link_fun();
}

void STag_route(void)
{
  STag_route_fun();
}

void ETag_route(void)
{
  ETag_route_fun();
}

void STag_route_element(void)
{
  STag_route_element_fun();
}

void ETag_route_element(void)
{
  ETag_route_element_fun();
}

void STag_process(void)
{
  STag_process_fun();
}

void ETag_process(void)
{
  ETag_process_fun();
}

void STag_argument(void)
{
  STag_argument_fun();
}

void ETag_argument(void)
{
  ETag_argument_fun();
}

void  surf_parse_open(const char *file) {
  if(!file) {
    WARN0("I hope you know what you're doing... you just gave me a NULL pointer!");
    return;
  }
  if(!surf_input_buffer_stack) 
    surf_input_buffer_stack = xbt_dynar_new(sizeof(YY_BUFFER_STATE),NULL);
  if(!surf_file_to_parse_stack) 
    surf_file_to_parse_stack = xbt_dynar_new(sizeof(FILE*),NULL);

  surf_file_to_parse = surf_fopen(file,"r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n",file);
  surf_input_buffer = surf_parse__create_buffer( surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void  surf_parse_close(void) {
  if(surf_input_buffer_stack) 
    xbt_dynar_free(&surf_input_buffer_stack);
  if(surf_file_to_parse_stack) 
    xbt_dynar_free(&surf_file_to_parse_stack);

  if(surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
  }
}


static int _surf_parse(void)
{
  return surf_parse_lex();
}

int_f_void_t surf_parse = _surf_parse;

void surf_parse_get_double(double *value,const char *string)
{ 
  int ret = 0;

  ret = sscanf(string, "%lg", value);
  xbt_assert2((ret==1), "Parse error line %d : %s not a number", surf_parse_lineno,
	      string);
}

void surf_parse_get_trace(tmgr_trace_t *trace, const char *string)
{
  if ((!string) || (strcmp(string, "") == 0))
    *trace = NULL;
  else
    *trace = tmgr_trace_new(string);
}

