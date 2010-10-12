/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_STUB_GENERATOR_H
#define GRAS_STUB_GENERATOR_H

#include <stdio.h>
#include <lauxlib.h>
#include <lualib.h>
#include "msg/msg.h"
#include "simdag/simdag.h"
#include <gras.h>
#include "xbt.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "xbt/function_types.h"
#include "xbt/log.h"
#include "surf/surfxml_parse.h"
#include "surf/surf_private.h"
#include "surf/surf.h"
#include "portable.h"           /* Needed for the time of the SIMIX convertion */
#include <stdarg.h>


xbt_dict_t process_function_set;
xbt_dynar_t process_list;
xbt_dict_t machine_set;

typedef struct s_process_t {
  int argc;
  char **argv;
  char *host;
} s_process_t;

char *warning;

void s_process_free(void *process);

/* UNIX files */
void generate_sim(const char *project);
void generate_rl(const char *project);
void generate_makefile_am(const char *project);
void generate_makefile_local(const char *project);

/* ********************************************************************************* */
/*                           Console functions                                       */
/* ********************************************************************************* */

typedef struct t_AS_attr {
  const char *id;
  const char *mode;
} AS_attr, *p_AS_attr;

typedef struct t_host_attr {
  //platform attribute
  // Mandatory attributes
  const char *id;
  double power_peak;
  // Optional attributes
  double power_scale;
  const char *power_trace;
  int state_initial;
  const char *state_trace;
  //deployment attribute
  const char *function;
  xbt_dynar_t args_list;
} host_attr, *p_host_attr;


typedef struct t_link_attr {
  //mandatory attributes
  const char *id;
  double bandwidth;
  double latency;
  // Optional attributes
  const char *bandwidth_trace;
  const char *latency_trace;
  const char *state_trace;
  int state_initial;
  int policy;
} link_attr, *p_link_attr;


typedef struct t_route_attr {
  const char *src_id;
  const char *dest_id;
  xbt_dynar_t links_id;

} route_attr, *p_route_attr;

// Public Functions

int console_add_host(lua_State*);
int console_add_link(lua_State*);
int console_add_route(lua_State*);
int console_add_AS(lua_State*);
int console_set_function(lua_State*);

int console_parse_platform(void);
int console_parse_application(void);
int console_parse_platform_wsL07(void);

#endif
