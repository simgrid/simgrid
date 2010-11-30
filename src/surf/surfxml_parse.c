/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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
xbt_dict_t traces_set_list = NULL;
xbt_dict_t trace_connect_list_host_avail = NULL;
xbt_dict_t trace_connect_list_power = NULL;
xbt_dict_t trace_connect_list_link_avail = NULL;
xbt_dict_t trace_connect_list_bandwidth = NULL;
xbt_dict_t trace_connect_list_latency = NULL;

/* This buffer is used to store the original buffer before substituting it by out own buffer. Useful for the foreach tag */
static xbt_dynar_t surfxml_bufferstack_stack = NULL;
int surfxml_bufferstack_size = 2048;

static char *old_buff = NULL;
static void surf_parse_error(char *msg);

unsigned int surfxml_buffer_stack_stack_ptr;
unsigned int surfxml_buffer_stack_stack[1024];


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
xbt_dynar_t STag_surfxml_link_ctn_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_ctn_cb_list = NULL;
xbt_dynar_t STag_surfxml_process_cb_list = NULL;
xbt_dynar_t ETag_surfxml_process_cb_list = NULL;
xbt_dynar_t STag_surfxml_argument_cb_list = NULL;
xbt_dynar_t ETag_surfxml_argument_cb_list = NULL;
xbt_dynar_t STag_surfxml_prop_cb_list = NULL;
xbt_dynar_t ETag_surfxml_prop_cb_list = NULL;
xbt_dynar_t STag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t ETag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_connect_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_connect_cb_list = NULL;
xbt_dynar_t STag_surfxml_random_cb_list = NULL;
xbt_dynar_t ETag_surfxml_random_cb_list = NULL;
xbt_dynar_t STag_surfxml_AS_cb_list = NULL;
xbt_dynar_t ETag_surfxml_AS_cb_list = NULL;
xbt_dynar_t STag_surfxml_ASroute_cb_list = NULL;
xbt_dynar_t ETag_surfxml_ASroute_cb_list = NULL;
xbt_dynar_t STag_surfxml_bypassRoute_cb_list = NULL;
xbt_dynar_t ETag_surfxml_bypassRoute_cb_list = NULL;
xbt_dynar_t STag_surfxml_config_cb_list = NULL;
xbt_dynar_t ETag_surfxml_config_cb_list = NULL;

/* store the current property set for any tag */
xbt_dict_t current_property_set = NULL;
/* dictionary of random generator data */
xbt_dict_t random_data_list = NULL;

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t);

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = NULL;

static void surf_parse_error(char *msg);

static void parse_Stag_trace(void);
static void parse_Etag_trace(void);
static void parse_Stag_trace_connect(void);

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
  xbt_dynar_free(&STag_surfxml_link_ctn_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_ctn_cb_list);
  xbt_dynar_free(&STag_surfxml_process_cb_list);
  xbt_dynar_free(&ETag_surfxml_process_cb_list);
  xbt_dynar_free(&STag_surfxml_argument_cb_list);
  xbt_dynar_free(&ETag_surfxml_argument_cb_list);
  xbt_dynar_free(&STag_surfxml_prop_cb_list);
  xbt_dynar_free(&ETag_surfxml_prop_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_connect_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_connect_cb_list);
  xbt_dynar_free(&STag_surfxml_random_cb_list);
  xbt_dynar_free(&ETag_surfxml_random_cb_list);
  xbt_dynar_free(&STag_surfxml_AS_cb_list);
  xbt_dynar_free(&ETag_surfxml_AS_cb_list);
  xbt_dynar_free(&STag_surfxml_ASroute_cb_list);
  xbt_dynar_free(&ETag_surfxml_ASroute_cb_list);
  xbt_dynar_free(&STag_surfxml_bypassRoute_cb_list);
  xbt_dynar_free(&ETag_surfxml_bypassRoute_cb_list);
  xbt_dynar_free(&STag_surfxml_cluster_cb_list);
  xbt_dynar_free(&ETag_surfxml_cluster_cb_list);
  xbt_dynar_free(&STag_surfxml_config_cb_list);
  xbt_dynar_free(&ETag_surfxml_config_cb_list);
}

void surf_parse_reset_parser(void)
{
  surf_parse_free_callbacks();
  STag_surfxml_platform_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_platform_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_link_ctn_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_link_ctn_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_process_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_process_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_argument_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_argument_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_trace_connect_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_trace_connect_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_AS_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_AS_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_ASroute_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_ASroute_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_bypassRoute_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_bypassRoute_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_cluster_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_cluster_cb_list =
      xbt_dynar_new(sizeof(void_f_void_t), NULL);
  STag_surfxml_config_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  ETag_surfxml_config_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
}

/* Stag and Etag parse functions */

void STag_surfxml_platform(void)
{
  double version = 0.0;

  sscanf(A_surfxml_platform_version, "%lg", &version);

  xbt_assert0((version >= 1.0), "******* BIG FAT WARNING *********\n "
      "You're using an ancient XML file.\n"
      "Since SimGrid 3.1, units are Bytes, Flops, and seconds "
      "instead of MBytes, MFlops and seconds.\n"

      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.\n"

      "Please check also out the SURF section of the ChangeLog for "
      "the 3.1 version for more information. \n"

      "Last, do not forget to also update your values for "
      "the calls to MSG_task_create (if any).");
  xbt_assert0((version >= 3.0), "******* BIG FAT WARNING *********\n "
      "You're using an old XML file.\n"
      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.");

  surfxml_call_cb_functions(STag_surfxml_platform_cb_list);

}

#define parse_method(type,name) \
void type##Tag_surfxml_##name(void) \
{ surfxml_call_cb_functions(type##Tag_surfxml_##name##_cb_list); }

parse_method(E, platform);
parse_method(S, host);
parse_method(E, host);
parse_method(S, router);
parse_method(E, router);
parse_method(S, link);
parse_method(E, link);
parse_method(S, route);
parse_method(E, route);
parse_method(S, link_ctn);
parse_method(E, link_ctn);
parse_method(S, process);
parse_method(E, process);
parse_method(S, argument);
parse_method(E, argument);
parse_method(S, prop);
parse_method(E, prop);
parse_method(S, trace);
parse_method(E, trace);
parse_method(S, trace_connect);
parse_method(E, trace_connect);
parse_method(S, random);
parse_method(E, random);
parse_method(S, AS);
parse_method(E, AS);
parse_method(S, ASroute);
parse_method(E, ASroute);
parse_method(S, bypassRoute);
parse_method(E, bypassRoute);
parse_method(S, cluster);
parse_method(E, cluster);
parse_method(S, config);
parse_method(E, config);

/* Open and Close parse file */

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
  surf_file_to_parse = surf_fopen(file, "r");
  xbt_assert1((surf_file_to_parse), "Unable to open \"%s\"\n", file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, 10);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close(void)
{
  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
    surf_file_to_parse = NULL; //Must be reset for Bypass
  }
}

/* Parse Function */

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

/* Aux parse functions */

void surfxml_add_callback(xbt_dynar_t cb_list, void_f_void_t function)
{
  xbt_dynar_push(cb_list, &function);
}

void surfxml_del_callback(xbt_dynar_t * p_cb_list, void_f_void_t function)
{
  xbt_dynar_t new_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
  unsigned int it;
  void_f_void_t func;
  xbt_dynar_foreach(*p_cb_list, it, func) {
    if (func != function)
      xbt_dynar_push(new_cb_list, &func);
  }
  xbt_dynar_free(p_cb_list);
  *p_cb_list = new_cb_list;
}

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t cb_list)
{
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(cb_list, iterator, fun) {
    (*fun) ();
  }
}

/* Init and free parse data */

static void init_data(void)
{
  if (!surfxml_bufferstack_stack)
    surfxml_bufferstack_stack = xbt_dynar_new(sizeof(char *), NULL);

  random_data_list = xbt_dict_new();
  traces_set_list = xbt_dict_new();
  trace_connect_list_host_avail = xbt_dict_new();
  trace_connect_list_power = xbt_dict_new();
  trace_connect_list_link_avail = xbt_dict_new();
  trace_connect_list_bandwidth = xbt_dict_new();
  trace_connect_list_latency = xbt_dict_new();

  surfxml_add_callback(STag_surfxml_random_cb_list, &init_randomness);
  surfxml_add_callback(ETag_surfxml_random_cb_list, &add_randomness);
  surfxml_add_callback(STag_surfxml_prop_cb_list, &parse_properties);
  surfxml_add_callback(STag_surfxml_trace_cb_list, &parse_Stag_trace);
  surfxml_add_callback(ETag_surfxml_trace_cb_list, &parse_Etag_trace);
  surfxml_add_callback(STag_surfxml_trace_connect_cb_list,
                       &parse_Stag_trace_connect);
}

static void free_data(void)
{
  xbt_dict_free(&trace_connect_list_host_avail);
  xbt_dict_free(&trace_connect_list_power);
  xbt_dict_free(&trace_connect_list_link_avail);
  xbt_dict_free(&trace_connect_list_bandwidth);
  xbt_dict_free(&trace_connect_list_latency);
  xbt_dict_free(&traces_set_list);
  xbt_dict_free(&random_data_list);
  xbt_dynar_free(&surfxml_bufferstack_stack);
}

/* Here start parse */

void parse_platform_file(const char *file)
{
  int parse_status;

  surfxml_buffer_stack_stack_ptr = 1;
  surfxml_buffer_stack_stack[0] = 0;

  surf_parse_open(file);
  init_data();
  parse_status = surf_parse();
  free_data();
  surf_parse_close();
  xbt_assert1(!parse_status, "Parse error in %s", file);
}

/* Prop tag functions */

void parse_properties(void)
{
  char *value = NULL;
  if (!current_property_set)
    current_property_set = xbt_dict_new();      // Maybe, it should be make a error
  value = xbt_strdup(A_surfxml_prop_value);
  xbt_dict_set(current_property_set, A_surfxml_prop_id, value, free);
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

static void parse_Stag_trace_connect(void)
{
  xbt_assert2(xbt_dict_get_or_null
              (traces_set_list, A_surfxml_trace_connect_trace),
              "Cannot connect trace %s to %s: trace unknown",
              A_surfxml_trace_connect_trace,
              A_surfxml_trace_connect_element);

  switch (A_surfxml_trace_connect_kind) {
  case A_surfxml_trace_connect_kind_HOST_AVAIL:
    xbt_dict_set(trace_connect_list_host_avail,
                 A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_POWER:
    xbt_dict_set(trace_connect_list_power, A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_LINK_AVAIL:
    xbt_dict_set(trace_connect_list_link_avail,
                 A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_BANDWIDTH:
    xbt_dict_set(trace_connect_list_bandwidth,
                 A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_LATENCY:
    xbt_dict_set(trace_connect_list_latency, A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  default:
    xbt_die(bprintf("Cannot connect trace %s to %s: kind of trace unknown",
                    A_surfxml_trace_connect_trace,
                    A_surfxml_trace_connect_element));
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
  xbt_dict_set(random_data_list, random_id, (void *) random,
               &xbt_free_ref);
}

/**
 * create CPU resource via CPU Model
 */
void surf_host_create_resource(char *name, double power_peak,
                               double power_scale,
                               tmgr_trace_t power_trace,
                               e_surf_resource_state_t state_initial,
                               tmgr_trace_t state_trace,
                               xbt_dict_t cpu_properties)
{
  return surf_cpu_model->extension.cpu.create_resource(name, power_peak,
                                                       power_scale,
                                                       power_trace,
                                                       state_initial,
                                                       state_trace,
                                                       cpu_properties);
}

/**
 * create CPU resource via worsktation_ptask_L07 model
 */

void surf_wsL07_host_create_resource(char *name, double power_peak,
                                     double power_scale,
                                     tmgr_trace_t power_trace,
                                     e_surf_resource_state_t state_initial,
                                     tmgr_trace_t state_trace,
                                     xbt_dict_t cpu_properties)
{
  surf_workstation_model->extension.workstation.cpu_create_resource(name,
                                                                    power_peak,
                                                                    power_scale,
                                                                    power_trace,
                                                                    state_initial,
                                                                    state_trace,
                                                                    cpu_properties);
}

/**
 * create link resource via network Model
 */
void surf_link_create_resource(char *name,
                               double bw_initial,
                               tmgr_trace_t bw_trace,
                               double lat_initial,
                               tmgr_trace_t lat_trace,
                               e_surf_resource_state_t
                               state_initial,
                               tmgr_trace_t state_trace,
                               e_surf_link_sharing_policy_t policy,
                               xbt_dict_t properties)
{
  return surf_network_model->extension.network.create_resource(name,
                                                               bw_initial,
                                                               bw_trace,
                                                               lat_initial,
                                                               lat_trace,
                                                               state_initial,
                                                               state_trace,
                                                               policy,
                                                               properties);
}

/**
 * create link resource via workstation_ptask_L07 model
 */

void surf_wsL07_link_create_resource(char *name,
                                     double bw_initial,
                                     tmgr_trace_t bw_trace,
                                     double lat_initial,
                                     tmgr_trace_t lat_trace,
                                     e_surf_resource_state_t
                                     state_initial,
                                     tmgr_trace_t state_trace,
                                     e_surf_link_sharing_policy_t
                                     policy, xbt_dict_t properties)
{
  return surf_workstation_model->extension.workstation.
      link_create_resource(name, bw_initial, bw_trace, lat_initial,
                           lat_trace, state_initial, state_trace, policy,
                           properties);
}

/**
 *
 *init new routing model component
 */

void surf_AS_new(const char *AS_id, const char *AS_mode)
{
  routing_AS_init(AS_id, AS_mode);
}

void surf_AS_finalize(const char *AS_id)
{
  routing_AS_end(AS_id);
}

/*
 * add host to the network element list
 */
void surf_route_add_host(const char *host_id)
{
  routing_add_host(host_id);
}

/**
 * set route
 */
void surf_routing_add_route(const char *src_id, const char *dst_id,
                            xbt_dynar_t links_id)
{
  unsigned int i;
  const char *link_id;
  routing_set_route(src_id, dst_id);
  xbt_dynar_foreach(links_id, i, link_id) {
    routing_add_link(link_id);
  }

  //store the route
  routing_store_route();
}

/**
 * Add Traces
 */
void surf_add_host_traces(void)
{
  return surf_cpu_model->extension.cpu.add_traces();
}

void surf_add_link_traces(void)
{
  return surf_network_model->extension.network.add_traces();
}

void surf_wsL07_add_traces(void)
{
  return surf_workstation_model->extension.workstation.add_traces();
}
