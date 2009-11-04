/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse_private.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf,
                                "Logging specific to the SURF parsing module");
#undef CLEANUP
#include "simgrid_dtd.c"

/* Initialize the parsing globals */
int route_action = 0;
xbt_dict_t traces_set_list = NULL;
//xbt_dynar_t traces_connect_list = NULL;
xbt_dict_t trace_connect_list_host_avail = NULL;
xbt_dict_t trace_connect_list_power = NULL;
xbt_dict_t trace_connect_list_link_avail = NULL;
xbt_dict_t trace_connect_list_bandwidth = NULL;
xbt_dict_t trace_connect_list_latency = NULL;

/* This buffer is used to store the original buffer before substituing it by out own buffer. Usefull for the foreach tag */
static xbt_dynar_t surfxml_bufferstack_stack = NULL;
int surfxml_bufferstack_size = 2048;
static char *old_buff = NULL;
static void surf_parse_error(char *msg);

void surfxml_bufferstack_push(int new)
{
  if (!new)
    old_buff = surfxml_bufferstack;
  else {
    xbt_dynar_push(surfxml_bufferstack_stack, &surfxml_bufferstack);
    surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
  }
}

void surfxml_bufferstack_pop(int new)
{
  if (!new)
    surfxml_bufferstack = old_buff;
  else {
    free(surfxml_bufferstack);
    xbt_dynar_pop(surfxml_bufferstack_stack, &surfxml_bufferstack);
  }
}

/* Stores the set name reffered to by the foreach tag */
static char *foreach_set_name;
static xbt_dynar_t main_STag_surfxml_host_cb_list = NULL;
static xbt_dynar_t main_STag_surfxml_link_cb_list = NULL;

/* make sure these symbols are defined as strong ones in this file so that the linked can resolve them */
xbt_dynar_t STag_surfxml_platform_cb_list = NULL;
xbt_dynar_t ETag_surfxml_platform_cb_list = NULL;
xbt_dynar_t STag_surfxml_host_cb_list = NULL;
xbt_dynar_t ETag_surfxml_host_cb_list = NULL;
xbt_dynar_t STag_surfxml_router_cb_list = NULL;
xbt_dynar_t ETag_surfxml_router_cb_list = NULL;
xbt_dynar_t STag_surfxml_link_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_cb_list = NULL;
xbt_dynar_t STag_surfxml_route_cb_list = NULL;
xbt_dynar_t ETag_surfxml_route_cb_list = NULL;
xbt_dynar_t STag_surfxml_link_c_ctn_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_c_ctn_cb_list = NULL;
xbt_dynar_t STag_surfxml_process_cb_list = NULL;
xbt_dynar_t ETag_surfxml_process_cb_list = NULL;
xbt_dynar_t STag_surfxml_argument_cb_list = NULL;
xbt_dynar_t ETag_surfxml_argument_cb_list = NULL;
xbt_dynar_t STag_surfxml_prop_cb_list = NULL;
xbt_dynar_t ETag_surfxml_prop_cb_list = NULL;
xbt_dynar_t STag_surfxml_set_cb_list = NULL;
xbt_dynar_t ETag_surfxml_set_cb_list = NULL;
xbt_dynar_t STag_surfxml_foreach_cb_list = NULL;
xbt_dynar_t ETag_surfxml_foreach_cb_list = NULL;
xbt_dynar_t STag_surfxml_route_c_multi_cb_list = NULL;
xbt_dynar_t ETag_surfxml_route_c_multi_cb_list = NULL;
xbt_dynar_t STag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t ETag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_c_connect_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_c_connect_cb_list = NULL;
xbt_dynar_t STag_surfxml_random_cb_list = NULL;
xbt_dynar_t ETag_surfxml_random_cb_list = NULL;

/* Stores the sets defined in the XML */
xbt_dict_t set_list = NULL;

xbt_dict_t current_property_set = NULL;

/* For the route:multi tag */
xbt_dict_t route_table = NULL;
xbt_dict_t route_multi_table = NULL;
xbt_dynar_t route_multi_elements = NULL;
static xbt_dynar_t route_link_list = NULL;
xbt_dynar_t links = NULL;
xbt_dynar_t keys = NULL;

xbt_dict_t random_data_list = NULL;

static xbt_dynar_t surf_input_buffer_stack = NULL;
static xbt_dynar_t surf_file_to_parse_stack = NULL;

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t);

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = NULL;

static void convert_route_multi_to_routes(void);
static void parse_route_elem(void);
static void parse_Stag_foreach(void);
static void parse_sets(void);
static void parse_Stag_route_multi(void);
static void parse_Etag_route_multi(void);
static void parse_Stag_trace(void);
static void parse_Etag_trace(void);
static void parse_Stag_trace_c_connect(void);
static void init_randomness(void);
static void add_randomness(void);

void surf_parse_free_callbacks(void)
{
  xbt_dynar_free(&STag_surfxml_platform_cb_list);
  xbt_dynar_free(&ETag_surfxml_platform_cb_list);
  xbt_dynar_free(&STag_surfxml_host_cb_list);
  xbt_dynar_free(&ETag_surfxml_host_cb_list);
  xbt_dynar_free(&STag_surfxml_router_cb_list);
  xbt_dynar_free(&ETag_surfxml_router_cb_list);
  xbt_dynar_free(&STag_surfxml_link_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_cb_list);
  xbt_dynar_free(&STag_surfxml_route_cb_list);
  xbt_dynar_free(&ETag_surfxml_route_cb_list);
  xbt_dynar_free(&STag_surfxml_link_c_ctn_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_c_ctn_cb_list);
  xbt_dynar_free(&STag_surfxml_process_cb_list);
  xbt_dynar_free(&ETag_surfxml_process_cb_list);
  xbt_dynar_free(&STag_surfxml_argument_cb_list);
  xbt_dynar_free(&ETag_surfxml_argument_cb_list);
  xbt_dynar_free(&STag_surfxml_prop_cb_list);
  xbt_dynar_free(&ETag_surfxml_prop_cb_list);
  xbt_dynar_free(&STag_surfxml_set_cb_list);
  xbt_dynar_free(&ETag_surfxml_set_cb_list);
  xbt_dynar_free(&STag_surfxml_foreach_cb_list);
  xbt_dynar_free(&ETag_surfxml_foreach_cb_list);
  xbt_dynar_free(&STag_surfxml_route_c_multi_cb_list);
  xbt_dynar_free(&ETag_surfxml_route_c_multi_cb_list);
  xbt_dynar_free(&STag_surfxml_cluster_cb_list);
  xbt_dynar_free(&ETag_surfxml_cluster_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_c_connect_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_c_connect_cb_list);
  xbt_dynar_free(&STag_surfxml_random_cb_list);
  xbt_dynar_free(&ETag_surfxml_random_cb_list);
}

void surf_parse_reset_parser(void)
{
  surf_parse_free_callbacks();
  STag_surfxml_platform_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_platform_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_link_c_ctn_cb_list =
    xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_link_c_ctn_cb_list =
    xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_process_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_process_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_argument_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_argument_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_set_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_set_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_foreach_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_foreach_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_route_c_multi_cb_list =
    xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_route_c_multi_cb_list =
    xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_cluster_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_cluster_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_trace_c_connect_cb_list =
    xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_trace_c_connect_cb_list =
    xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
}

void STag_surfxml_include(void)
{
  xbt_dynar_push(surf_input_buffer_stack, &surf_input_buffer);
  xbt_dynar_push(surf_file_to_parse_stack, &surf_file_to_parse);

  surf_file_to_parse = surf_fopen(A_surfxml_include_file, "r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n",
              A_surfxml_include_file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  printf("STAG\n");
  fflush(NULL);
}

void ETag_surfxml_include(void)
{
  printf("ETAG\n");
  fflush(NULL);
  surf_parse__delete_buffer(surf_input_buffer);
  fclose(surf_file_to_parse);
  xbt_dynar_pop(surf_file_to_parse_stack, &surf_file_to_parse);
  xbt_dynar_pop(surf_input_buffer_stack, &surf_input_buffer);
}

void STag_surfxml_platform(void)
{
  double version = 0.0;

  sscanf(A_surfxml_platform_version, "%lg", &version);

  xbt_assert0((version >= 1.0), "******* BIG FAT WARNING *********\n "
              "You're using an ancient XML file. "
              "Since SimGrid 3.1, units are Bytes, Flops, and seconds "
              "instead of MBytes, MFlops and seconds. "
              "A script (surfxml_update.pl) to help you convert your old "
              "platform files "
              "is available in the contrib/platform_generation directory "
              "of the simgrid repository. Please check also out the "
              "SURF section of the ChangeLog for the 3.1 version. "
              "Last, do not forget to also update your values for "
              "the calls to MSG_task_create (if any).");
  xbt_assert0((version >= 2.0), "******* BIG FAT WARNING *********\n "
              "You're using an old XML file. "
              "A script (surfxml_update.pl) to help you convert your old "
              "platform files "
              "is available in the contrib/platform_generation directory "
              "of the simgrid repository.");

  surfxml_call_cb_functions(STag_surfxml_platform_cb_list);

}

void ETag_surfxml_platform(void)
{
  convert_route_multi_to_routes();

  surfxml_call_cb_functions(ETag_surfxml_platform_cb_list);

  xbt_dict_free(&random_data_list);
}

void STag_surfxml_host(void)
{
  surfxml_call_cb_functions(STag_surfxml_host_cb_list);
}

void ETag_surfxml_host(void)
{
  surfxml_call_cb_functions(ETag_surfxml_host_cb_list);
}

void STag_surfxml_router(void)
{
  surfxml_call_cb_functions(STag_surfxml_router_cb_list);
}

void ETag_surfxml_router(void)
{
  surfxml_call_cb_functions(ETag_surfxml_router_cb_list);
}

void STag_surfxml_link(void)
{
  surfxml_call_cb_functions(STag_surfxml_link_cb_list);
}

void ETag_surfxml_link(void)
{
  surfxml_call_cb_functions(ETag_surfxml_link_cb_list);
}

void STag_surfxml_route(void)
{
  surfxml_call_cb_functions(STag_surfxml_route_cb_list);
}

void ETag_surfxml_route(void)
{
  surfxml_call_cb_functions(ETag_surfxml_route_cb_list);
}

void STag_surfxml_link_c_ctn(void)
{
  surfxml_call_cb_functions(STag_surfxml_link_c_ctn_cb_list);
}

void ETag_surfxml_link_c_ctn(void)
{
  surfxml_call_cb_functions(ETag_surfxml_link_c_ctn_cb_list);
}

void STag_surfxml_process(void)
{
  surfxml_call_cb_functions(STag_surfxml_process_cb_list);
}

void ETag_surfxml_process(void)
{
  surfxml_call_cb_functions(ETag_surfxml_process_cb_list);
}

void STag_surfxml_argument(void)
{
  surfxml_call_cb_functions(STag_surfxml_argument_cb_list);
}

void ETag_surfxml_argument(void)
{
  surfxml_call_cb_functions(ETag_surfxml_argument_cb_list);
}

void STag_surfxml_prop(void)
{
  surfxml_call_cb_functions(STag_surfxml_prop_cb_list);
}

void ETag_surfxml_prop(void)
{
  surfxml_call_cb_functions(ETag_surfxml_prop_cb_list);
}

void STag_surfxml_set(void)
{
  surfxml_call_cb_functions(STag_surfxml_set_cb_list);
}

void ETag_surfxml_set(void)
{
  surfxml_call_cb_functions(ETag_surfxml_set_cb_list);
}

void STag_surfxml_foreach(void)
{
  /* Save the current buffer */
  surfxml_bufferstack_push(0);
  surfxml_call_cb_functions(STag_surfxml_foreach_cb_list);
}

void ETag_surfxml_foreach(void)
{
  surfxml_call_cb_functions(ETag_surfxml_foreach_cb_list);

  /* free the temporary dynar and restore original */
  xbt_dynar_free(&STag_surfxml_host_cb_list);
  STag_surfxml_host_cb_list = main_STag_surfxml_host_cb_list;

  /* free the temporary dynar and restore original */
  xbt_dynar_free(&STag_surfxml_link_cb_list);
  STag_surfxml_link_cb_list = main_STag_surfxml_link_cb_list;
}

void STag_surfxml_route_c_multi(void)
{
  surfxml_call_cb_functions(STag_surfxml_route_c_multi_cb_list);
}

void ETag_surfxml_route_c_multi(void)
{
  surfxml_call_cb_functions(ETag_surfxml_route_c_multi_cb_list);
}

void STag_surfxml_cluster(void)
{
  surfxml_call_cb_functions(STag_surfxml_cluster_cb_list);
}

void ETag_surfxml_cluster(void)
{
  surfxml_call_cb_functions(ETag_surfxml_cluster_cb_list);
}

void STag_surfxml_trace(void)
{
  surfxml_call_cb_functions(STag_surfxml_trace_cb_list);
}

void ETag_surfxml_trace(void)
{
  surfxml_call_cb_functions(ETag_surfxml_trace_cb_list);
}

void STag_surfxml_trace_c_connect(void)
{
  surfxml_call_cb_functions(STag_surfxml_trace_c_connect_cb_list);
}

void ETag_surfxml_trace_c_connect(void)
{
  surfxml_call_cb_functions(ETag_surfxml_trace_c_connect_cb_list);
}

void STag_surfxml_random(void)
{
  surfxml_call_cb_functions(STag_surfxml_random_cb_list);
}

void ETag_surfxml_random(void)
{
  surfxml_call_cb_functions(ETag_surfxml_random_cb_list);
}

void surf_parse_open(const char *file)
{
  static int warned = 0;        /* warn only once */
  if (!file) {
    if (!warned) {
      WARN0
        ("Bypassing the XML parser since surf_parse_open received a NULL pointer. If it is not what you want, go fix your code.");
      warned = 1;
    }
    return;
  }
  if (!surf_input_buffer_stack)
    surf_input_buffer_stack = xbt_dynar_new(sizeof(YY_BUFFER_STATE), NULL);
  if (!surf_file_to_parse_stack)
    surf_file_to_parse_stack = xbt_dynar_new(sizeof(FILE *), NULL);

  surf_file_to_parse = surf_fopen(file, "r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n", file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close(void)
{
  if (surf_input_buffer_stack)
    xbt_dynar_free(&surf_input_buffer_stack);
  if (surf_file_to_parse_stack)
    xbt_dynar_free(&surf_file_to_parse_stack);

  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
  }
}


static int _surf_parse(void)
{
  return surf_parse_lex();
}

int_f_void_t surf_parse = _surf_parse;

void surf_parse_error(char *msg)
{
  fprintf(stderr, "Parse error on line %d: %s\n", surf_parse_lineno, msg);
  abort();
}


void surf_parse_get_double(double *value, const char *string)
{
  int ret = 0;

  ret = sscanf(string, "%lg", value);
  if (ret != 1)
    surf_parse_error(bprintf("%s is not a double", string));
}

void surf_parse_get_int(int *value, const char *string)
{
  int ret = 0;

  ret = sscanf(string, "%d", value);
  if (ret != 1)
    surf_parse_error(bprintf("%s is not an integer", string));
}

void parse_properties(void)
{
  char *value = NULL;

  if (!current_property_set)
    current_property_set = xbt_dict_new();

  value = xbt_strdup(A_surfxml_prop_value);
  xbt_dict_set(current_property_set, A_surfxml_prop_id, value, free);
}

void surfxml_add_callback(xbt_dynar_t cb_list, void_f_void_t function)
{
  xbt_dynar_push(cb_list, &function);
}

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t cb_list)
{
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(cb_list, iterator, fun) {
    (*fun) ();
  }
}

static void parse_route_set_endpoints(void)
{
  route_link_list = xbt_dynar_new(sizeof(char *), NULL);
}

static void init_data(void)
{
  xbt_dict_free(&route_table);
  xbt_dynar_free(&route_link_list);
  route_table = xbt_dict_new();

  if (!surfxml_bufferstack_stack)
    surfxml_bufferstack_stack = xbt_dynar_new(sizeof(char *), NULL);
  route_multi_table = xbt_dict_new();
  route_multi_elements = xbt_dynar_new(sizeof(char *), NULL);
  traces_set_list = xbt_dict_new();
  if (set_list == NULL)
    set_list = xbt_dict_new();

  trace_connect_list_host_avail = xbt_dict_new();
  trace_connect_list_power = xbt_dict_new();
  trace_connect_list_link_avail = xbt_dict_new();
  trace_connect_list_bandwidth = xbt_dict_new();
  trace_connect_list_latency = xbt_dict_new();

  random_data_list = xbt_dict_new();
  surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties);
  surfxml_add_callback(ETag_surfxml_link_c_ctn_cb_list, &parse_route_elem);
  surfxml_add_callback(STag_surfxml_route_cb_list,
                       &parse_route_set_endpoints);
  surfxml_add_callback(STag_surfxml_set_cb_list, &parse_sets);
  surfxml_add_callback(STag_surfxml_route_c_multi_cb_list,
                       &parse_Stag_route_multi);
  surfxml_add_callback(ETag_surfxml_route_c_multi_cb_list,
                       &parse_Etag_route_multi);
  surfxml_add_callback(STag_surfxml_foreach_cb_list, &parse_Stag_foreach);
  surfxml_add_callback(STag_surfxml_trace_cb_list, &parse_Stag_trace);
  surfxml_add_callback(ETag_surfxml_trace_cb_list, &parse_Etag_trace);
  surfxml_add_callback(STag_surfxml_trace_c_connect_cb_list,
                       &parse_Stag_trace_c_connect);
  surfxml_add_callback(STag_surfxml_random_cb_list, &init_randomness);
  surfxml_add_callback(ETag_surfxml_random_cb_list, &add_randomness);
}

static void free_data(void)
{
  char *key, *data;
  xbt_dict_cursor_t cursor = NULL;
  char *name;
  unsigned int cpt = 0;

  xbt_dict_foreach(route_table, cursor, key, data) {
    xbt_dynar_t links = (xbt_dynar_t) data;
    char *name;
    unsigned int cpt = 0;

    xbt_dynar_foreach(links, cpt, name) free(name);
    xbt_dynar_free(&links);
  }
  xbt_dict_free(&route_table);
  route_link_list = NULL;

  xbt_dict_free(&route_multi_table);

  xbt_dynar_foreach(route_multi_elements, cpt, name) free(name);
  xbt_dynar_free(&route_multi_elements);

  xbt_dict_foreach(set_list, cursor, key, data) {
    xbt_dynar_t set = (xbt_dynar_t) data;

    xbt_dynar_foreach(set, cpt, name) free(name);
    xbt_dynar_free(&set);
  }
  xbt_dict_free(&set_list);

  xbt_dynar_free(&surfxml_bufferstack_stack);

  xbt_dict_free(&traces_set_list);
  xbt_dict_free(&trace_connect_list_host_avail);
  xbt_dict_free(&trace_connect_list_power);
  xbt_dict_free(&trace_connect_list_link_avail);
  xbt_dict_free(&trace_connect_list_bandwidth);
  xbt_dict_free(&trace_connect_list_latency);
  xbt_dict_free(&traces_set_list);
}

void parse_platform_file(const char *file)
{
  surf_parse_open(file);
  init_data();
  xbt_assert1((!(*surf_parse) ()), "Parse error in %s", file);
  free_data();
  surf_parse_close();
}

/* Functions to bypass route tag. Used by the route:multi tag */

static void parse_make_temporary_route(const char *src, const char *dst,
                                       int action)
{
  int AX_ptr = 0;

  A_surfxml_route_action = action;
  SURFXML_BUFFER_SET(route_src, src);
  SURFXML_BUFFER_SET(route_dst, dst);
}

/* Functions for the sets and foreach tags */

static void parse_sets(void)
{
  char *id, *suffix, *prefix, *radical;
  int start, end;
  xbt_dynar_t radical_elements;
  xbt_dynar_t radical_ends;
  xbt_dynar_t current_set;
  char *value, *groups;
  int i;
  unsigned int iter;

  id = xbt_strdup(A_surfxml_set_id);
  prefix = xbt_strdup(A_surfxml_set_prefix);
  suffix = xbt_strdup(A_surfxml_set_suffix);
  radical = xbt_strdup(A_surfxml_set_radical);

  if (xbt_dict_get_or_null(set_list, id))
    surf_parse_error(bprintf
                     ("Set '%s' declared several times in the platform file.",
                      id));

  current_set = xbt_dynar_new(sizeof(char *), NULL);

  radical_elements = xbt_str_split(radical, ",");
  xbt_dynar_foreach(radical_elements, iter, groups) {

    radical_ends = xbt_str_split(groups, "-");
    switch (xbt_dynar_length(radical_ends)) {
    case 1:
      surf_parse_get_int(&start, xbt_dynar_get_as(radical_ends, 0, char *));
      value = bprintf("%s%d%s", prefix, start, suffix);
      xbt_dynar_push(current_set, &value);
      break;

    case 2:

      surf_parse_get_int(&start, xbt_dynar_get_as(radical_ends, 0, char *));
      surf_parse_get_int(&end, xbt_dynar_get_as(radical_ends, 1, char *));


      for (i = start; i <= end; i++) {
        value = bprintf("%s%d%s", prefix, i, suffix);
        xbt_dynar_push(current_set, &value);
      }
      break;

    default:
      surf_parse_error(xbt_strdup("Malformed radical"));
    }

    xbt_dynar_free(&radical_ends);
  }

  xbt_dict_set(set_list, id, current_set, NULL);

  xbt_dynar_free(&radical_elements);
  free(radical);
  free(suffix);
  free(prefix);
  free(id);
}

static void parse_host_foreach(void){
  xbt_dynar_t names = NULL;
  unsigned int cpt = 0;
  char *name;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;

  const char *surfxml_host_power = A_surfxml_host_power;
  const char *surfxml_host_availability = A_surfxml_host_availability;
  const char *surfxml_host_availability_file = A_surfxml_host_availability_file;
  const char *surfxml_host_state_file = A_surfxml_host_state_file;


  xbt_dict_t cluster_host_props = current_property_set;

  names = xbt_dict_get_or_null(set_list, foreach_set_name);
  if (!names)
    surf_parse_error(bprintf("Set name '%s' used in <foreach> not found.",
                             foreach_set_name));
  if (strcmp(A_surfxml_host_id, "$1"))
    surf_parse_error(bprintf
                     ("The host id within <foreach> should point to the foreach set_id (use $1 instead of %s)",
                      A_surfxml_host_id));


  /* foreach name in set call the main host callback */
  xbt_dynar_foreach(names, cpt, name) {
    int AX_ptr = 0; /* needed by the SURFXML_BUFFER_SET macro */

    surfxml_bufferstack_push(1);

    SURFXML_BUFFER_SET(host_id, name);
    SURFXML_BUFFER_SET(host_power, surfxml_host_power /*hostPower */ );
    SURFXML_BUFFER_SET(host_availability, surfxml_host_availability);
    SURFXML_BUFFER_SET(host_availability_file, surfxml_host_availability_file);
    SURFXML_BUFFER_SET(host_state_file, surfxml_host_state_file);

    surfxml_call_cb_functions(main_STag_surfxml_host_cb_list);

    xbt_dict_foreach(cluster_host_props, cursor, key, data) {
      xbt_dict_set(current_property_set, xbt_strdup(key), xbt_strdup(data),
                   free);
    }

    /* Call the (unmodified) callbacks of </host>, if any */
    surfxml_call_cb_functions(ETag_surfxml_host_cb_list);
    surfxml_bufferstack_pop(1);
  }

  current_property_set = xbt_dict_new();

  surfxml_bufferstack_pop(0);
}

static void parse_link_foreach(void) {
  const char *surfxml_link_bandwidth = A_surfxml_link_bandwidth;
  const char *surfxml_link_bandwidth_file = A_surfxml_link_bandwidth_file;
  const char *surfxml_link_latency = A_surfxml_link_latency;
  const char *surfxml_link_latency_file = A_surfxml_link_latency_file;
  const char *surfxml_link_state_file = A_surfxml_link_state_file;

  xbt_dynar_t names = NULL;
  unsigned int cpt = 0;
  char *name;
  xbt_dict_cursor_t cursor = NULL;
  char *key, *data;

  xbt_dict_t cluster_link_props = current_property_set;

  names = xbt_dict_get_or_null(set_list, foreach_set_name);
  if (!names)
    surf_parse_error(bprintf("Set name '%s' used in <foreach> not found.",
                             foreach_set_name));
  if (strcmp(A_surfxml_link_id, "$1"))
    surf_parse_error(bprintf
                     ("The host id within <foreach> should point to the foreach set_id (use $1 instead of %s)",
                      A_surfxml_link_id));

  /* for each name in set call the main link callback */
  xbt_dynar_foreach(names, cpt, name) {
    int AX_ptr = 0; /* needed by the SURFXML_BUFFER_SET */

    surfxml_bufferstack_push(1);

    SURFXML_BUFFER_SET(link_id, name);
    SURFXML_BUFFER_SET(link_bandwidth, surfxml_link_bandwidth);
    SURFXML_BUFFER_SET(link_bandwidth_file, surfxml_link_bandwidth_file);
    SURFXML_BUFFER_SET(link_latency, surfxml_link_latency);
    SURFXML_BUFFER_SET(link_latency_file, surfxml_link_latency_file);
    SURFXML_BUFFER_SET(link_state_file, surfxml_link_state_file);

    surfxml_call_cb_functions(main_STag_surfxml_link_cb_list);

    xbt_dict_foreach(cluster_link_props, cursor, key, data) {
      xbt_dict_set(current_property_set, xbt_strdup(key), xbt_strdup(data),
                   free);
    }

    /* Call the (unmodified) callbacks of </link>, if any */
    surfxml_call_cb_functions(ETag_surfxml_link_cb_list);
    surfxml_bufferstack_pop(1);
  }

  current_property_set = xbt_dict_new();

  surfxml_bufferstack_pop(0);
  free(foreach_set_name);
  foreach_set_name = NULL;
}

static void parse_Stag_foreach(void)
{
  /* save the host & link callbacks */
  main_STag_surfxml_host_cb_list = STag_surfxml_host_cb_list;
  main_STag_surfxml_link_cb_list = STag_surfxml_link_cb_list;

  /* redefine host & link callbacks to be used only by the foreach tag */
  STag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);

  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_host_foreach);
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_link_foreach);

  /* get set name */
  foreach_set_name = xbt_strdup(A_surfxml_foreach_set_id);
}

/* Route:multi functions */

static int route_multi_size = 0;
static char *src_name, *dst_name;
static int is_symmetric_route;

static void parse_route_elem(void)
{
  char *val;

  val = xbt_strdup(A_surfxml_link_c_ctn_id);

  xbt_dynar_push(route_link_list, &val);
  //INFO2("Push %s (size now:%ld)",val,xbt_dynar_length(route_link_list));
}

static void parse_Stag_route_multi(void)
{
  src_name = xbt_strdup(A_surfxml_route_c_multi_src);
  dst_name = xbt_strdup(A_surfxml_route_c_multi_dst);
  route_action = A_surfxml_route_c_multi_action;
  is_symmetric_route = A_surfxml_route_c_multi_symmetric;
  route_multi_size++;

  route_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

/*
   This function is used to append or override the contents of an already existing route in the case a new one with its name is found.
   The decision is based upon the value of action specified in the xml route:multi attribute action
 */
void manage_route(xbt_dict_t routing_table, const char *route_name,
                  int action, int isMultiRoute)
{
  unsigned int cpt;
  xbt_dynar_t links;
  char *value;

  /* get already existing list if it exists */
  links = xbt_dict_get_or_null(routing_table, route_name);
  DEBUG3("ROUTE: %s (action:%s; len:%ld)", route_name,
      (action==A_surfxml_route_action_OVERRIDE?"override":(
          action==A_surfxml_route_action_PREPEND?"prepend":"postpend")),
       (links?xbt_dynar_length(links):0));

  if (links != NULL) {
    switch (action) {
    case A_surfxml_route_action_PREPEND:       /* add existing links at the end; route_link_list + links */
      xbt_dynar_foreach(links, cpt, value) {
        xbt_dynar_push(route_link_list, &value);
      }
      xbt_dynar_free(&links);
      break;
    case A_surfxml_route_action_POSTPEND:      /* add existing links in front; links + route_link_list */
      xbt_dynar_foreach(route_link_list, cpt, value) {
        xbt_dynar_push(links, &value);
      }
      xbt_dynar_free(&route_link_list);
      route_link_list = links;
      break;
    case A_surfxml_route_action_OVERRIDE:
      xbt_dynar_free(&links);
      break;
    default:
      xbt_die(bprintf("While dealing with routes of %s, got action=%d. Please report this bug.",
          route_name,action));
      break;
    }
  }
  /* this is the final route; do not add if name is a set; add only if name is in set list */
  if (!isMultiRoute) {
    xbt_dict_set(routing_table, route_name, route_link_list, NULL);
  }
}

static void parse_Etag_route_multi(void)
{
  char *route_name;

  route_name =
    bprintf("%s#%s#%d#%d#%d", src_name, dst_name, route_action,
            is_symmetric_route, route_multi_size);

  xbt_dynar_push(route_multi_elements, &route_name);

  /* Add route */
  xbt_dict_set(route_multi_table, route_name, route_link_list, NULL);
  /* add symmetric if it is the case */
  if (is_symmetric_route == 1) {
    char *symmetric_name =
      bprintf("%s#%s#%d#%d#%d", dst_name, src_name, route_action,
              !is_symmetric_route, route_multi_size);

    xbt_dict_set(route_multi_table, symmetric_name, route_link_list, NULL);
    xbt_dynar_push(route_multi_elements, &symmetric_name);
    is_symmetric_route = 0;
  }
  free(src_name);
  free(dst_name);
}

static void add_multi_links(const char *src, const char *dst,
                            xbt_dynar_t links, const char *src_name,
                            const char *dst_name)
{
  unsigned int cpt;
  char *value, *val;

  surfxml_bufferstack_push(1);

  parse_make_temporary_route(src_name, dst_name, route_action);
  surfxml_call_cb_functions(STag_surfxml_route_cb_list);
  DEBUG2("\tADDING ROUTE: %s -> %s", src_name, dst_name);
  /* Build link list */
  xbt_dynar_foreach(links, cpt, value) {
    if (strcmp(value, src) == 0)
      val = xbt_strdup(src_name);
    else if (strcmp(value, dst) == 0)
      val = xbt_strdup(dst_name);
    else if (strcmp(value, "$dst") == 0)
      val = xbt_strdup(dst_name);
    else if (strcmp(value, "$src") == 0)
      val = xbt_strdup(src_name);
    else
      val = xbt_strdup(value);
    DEBUG1("\t\tELEMENT: %s", val);
    xbt_dynar_push(route_link_list, &val);
  }
  surfxml_call_cb_functions(ETag_surfxml_route_cb_list);
  surfxml_bufferstack_pop(1);
}

static void convert_route_multi_to_routes(void)
{
  xbt_dict_cursor_t cursor_w;
  int symmetric;
  unsigned int cpt, cpt2, cursor;
  char *src_host_name, *dst_host_name, *key, *src, *dst, *val, *key_w,
    *data_w;
  const char *sep = "#";
  xbt_dict_t set = NULL;
  xbt_dynar_t src_names = NULL, dst_names = NULL, links;

  if (!route_multi_elements)
    return;

  if (surf_cpu_model)
    set = surf_model_resource_set(surf_cpu_model);
  if (surf_workstation_model != NULL &&
      surf_model_resource_set(surf_workstation_model) != NULL &&
      xbt_dict_length(surf_model_resource_set(surf_workstation_model)) > 0)
    set = surf_model_resource_set(surf_workstation_model);


  surfxml_bufferstack_push(0);
  /* Get all routes in the exact order they were entered in the platform file */
  xbt_dynar_foreach(route_multi_elements, cursor, key) {
    /* Get links for the route */
    links = (xbt_dynar_t) xbt_dict_get_or_null(route_multi_table, key);
    keys = xbt_str_split_str(key, sep);
    /* Get route ends */
    src = xbt_dynar_get_as(keys, 0, char *);
    dst = xbt_dynar_get_as(keys, 1, char *);
    route_action = atoi(xbt_dynar_get_as(keys, 2, char *));
    symmetric = atoi(xbt_dynar_get_as(keys, 3, char *));

    /* Create the dynar of src and dst hosts for the new routes */
    /* NOTE: src and dst can be either set names or simple host names */
    src_names = (xbt_dynar_t) xbt_dict_get_or_null(set_list, src);
    dst_names = (xbt_dynar_t) xbt_dict_get_or_null(set_list, dst);
    /* Add to dynar even if they are simple names */
    if (src_names == NULL) {
      src_names = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
      val = xbt_strdup(src);
      xbt_dynar_push(src_names, &val);
      if (strcmp(val, "$*") != 0 && NULL == xbt_dict_get_or_null(set, val))
        THROW3(unknown_error, 0,
               "(In route:multi (%s -> %s) source %s does not exist (not a set or a host)",
               src, dst, src);
    }
    if (dst_names == NULL) {
      dst_names = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
      val = xbt_strdup(dst);
      if (strcmp(val, "$*") != 0 && NULL == xbt_dict_get_or_null(set, val))
        THROW3(unknown_error, 0,
               "(In route:multi (%s -> %s) destination %s does not exist (not a set or a host)",
               src, dst, dst);
      xbt_dynar_push(dst_names, &val);
    }

    /* Build the routes */
    DEBUG2("ADDING MULTI ROUTE: %s -> %s", xbt_dynar_get_as(keys, 0, char *),
           xbt_dynar_get_as(keys, 1, char *));
    xbt_dynar_foreach(src_names, cpt, src_host_name) {
      xbt_dynar_foreach(dst_names, cpt2, dst_host_name) {
        /* If dst is $* then set this route to have its dst point to all hosts */
        if (strcmp(src_host_name, "$*") != 0
            && strcmp(dst_host_name, "$*") == 0) {
          xbt_dict_foreach(set, cursor_w, key_w, data_w) {
            //int n = xbt_dynar_member(src_names, (char*)key_w);
            add_multi_links(src, dst, links, src_host_name, key_w);
          }
        }
        /* If src is $* then set this route to have its dst point to all hosts */
        if (strcmp(src_host_name, "$*") == 0
            && strcmp(dst_host_name, "$*") != 0) {
          xbt_dict_foreach(set, cursor_w, key_w, data_w) {
            // if (!symmetric || (symmetric && !contains(dst_names, key_w)))
            add_multi_links(src, dst, links, key_w, dst_host_name);
          }
        }
        /* if none of them are equal to $* */
        if (strcmp(src_host_name, "$*") != 0
            && strcmp(dst_host_name, "$*") != 0) {
          add_multi_links(src, dst, links, src_host_name, dst_host_name);
        }
      }
    }
    xbt_dynar_free(&keys);
  }
  surfxml_bufferstack_pop(0);
}


/* Trace management functions */

static double trace_periodicity = -1.0;
static char *trace_file = NULL;
static char *trace_id;

static void parse_Stag_trace(void)
{
  trace_id = strdup(A_surfxml_trace_id);
  trace_file = strdup(A_surfxml_trace_file);
  surf_parse_get_double(&trace_periodicity, A_surfxml_trace_periodicity);
}

static void parse_Etag_trace(void)
{
  tmgr_trace_t trace;
  if (!trace_file || strcmp(trace_file, "") != 0) {
    trace = tmgr_trace_new(trace_file);
  } else {
    if (strcmp(surfxml_pcdata, "") == 0)
      trace = NULL;
    else
      trace =
        tmgr_trace_new_from_string(trace_id, surfxml_pcdata,
                                   trace_periodicity);
  }
  xbt_dict_set(traces_set_list, trace_id, (void *) trace, NULL);
}

static void parse_Stag_trace_c_connect(void)
{
  xbt_assert2(xbt_dict_get_or_null
              (traces_set_list, A_surfxml_trace_c_connect_trace),
              "Cannot connect trace %s to %s: trace unknown",
              A_surfxml_trace_c_connect_trace,
              A_surfxml_trace_c_connect_element);

  switch (A_surfxml_trace_c_connect_kind) {
  case A_surfxml_trace_c_connect_kind_HOST_AVAIL:
    xbt_dict_set(trace_connect_list_host_avail,
                 A_surfxml_trace_c_connect_trace,
                 xbt_strdup(A_surfxml_trace_c_connect_element), free);
    break;
  case A_surfxml_trace_c_connect_kind_POWER:
    xbt_dict_set(trace_connect_list_power, A_surfxml_trace_c_connect_trace,
                 xbt_strdup(A_surfxml_trace_c_connect_element), free);
    break;
  case A_surfxml_trace_c_connect_kind_LINK_AVAIL:
    xbt_dict_set(trace_connect_list_link_avail,
                 A_surfxml_trace_c_connect_trace,
                 xbt_strdup(A_surfxml_trace_c_connect_element), free);
    break;
  case A_surfxml_trace_c_connect_kind_BANDWIDTH:
    xbt_dict_set(trace_connect_list_bandwidth,
                 A_surfxml_trace_c_connect_trace,
                 xbt_strdup(A_surfxml_trace_c_connect_element), free);
    break;
  case A_surfxml_trace_c_connect_kind_LATENCY:
    xbt_dict_set(trace_connect_list_latency, A_surfxml_trace_c_connect_trace,
                 xbt_strdup(A_surfxml_trace_c_connect_element), free);
    break;
  default:
    xbt_die(bprintf("Cannot connect trace %s to %s: kind of trace unknown",
                    A_surfxml_trace_c_connect_trace,
                    A_surfxml_trace_c_connect_element));
  }
}

/* Random tag functions */

double get_cpu_power(const char *power)
{
  double power_scale = 0.0;
  const char *p, *q;
  char *generator;
  random_data_t random = NULL;
  /* randomness is inserted like this: power="$rand(my_random)" */
  if (((p = strstr(power, "$rand(")) != NULL)
      && ((q = strstr(power, ")")) != NULL)) {
    if (p < q) {
      generator = xbt_malloc(q - (p + 6) + 1);
      memcpy(generator, p + 6, q - (p + 6));
      generator[q - (p + 6)] = '\0';
      xbt_assert1((random =
                   xbt_dict_get_or_null(random_data_list, generator)),
                  "Random generator %s undefined", generator);
      power_scale = random_generate(random);
    }
  } else {
    surf_parse_get_double(&power_scale, power);
  }
  return power_scale;
}

double random_min, random_max, random_mean, random_std_deviation,
  random_generator;
char *random_id;

static void init_randomness(void)
{
  random_id = A_surfxml_random_id;
  surf_parse_get_double(&random_min, A_surfxml_random_min);
  surf_parse_get_double(&random_max, A_surfxml_random_max);
  surf_parse_get_double(&random_mean, A_surfxml_random_mean);
  surf_parse_get_double(&random_std_deviation,
                        A_surfxml_random_std_deviation);
  random_generator = A_surfxml_random_generator;
}

static void add_randomness(void)
{
  /* If needed aditional properties can be added by using the prop tag */
  random_data_t random =
    random_new(random_generator, 0, random_min, random_max, random_mean,
               random_std_deviation);
  xbt_dict_set(random_data_list, random_id, (void *) random, NULL);
}
