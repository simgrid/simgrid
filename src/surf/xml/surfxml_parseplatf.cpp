/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/instr/instr_private.h" // TRACE_start(). FIXME: remove by subscribing tracing to the surf signals

#include "src/surf/xml/platf.hpp"

#if HAVE_LUA
extern "C" {
#include "src/bindings/lua/simgrid_lua.h"

#include <lua.h>                /* Always include this when calling Lua */
#include <lauxlib.h>            /* Always include this when calling Lua */
#include <lualib.h>             /* Prototype for luaL_openlibs(), */
}
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

/*
 *  Allow the cluster tag to mess with the parsing buffer.
 * (this will probably become obsolete once the cluster tag do not mess with the parsing callbacks directly)
 */

/* This buffer is used to store the original buffer before substituting it by out own buffer. Useful for the cluster tag */
static xbt_dynar_t surfxml_bufferstack_stack = NULL;
int surfxml_bufferstack_size = 2048;

static char *old_buff = NULL;

XBT_IMPORT_NO_EXPORT(unsigned int) surfxml_buffer_stack_stack_ptr;
XBT_IMPORT_NO_EXPORT(unsigned int) surfxml_buffer_stack_stack[1024];

void surfxml_bufferstack_push(int new_one)
{
  if (!new_one)
    old_buff = surfxml_bufferstack;
  else {
    xbt_dynar_push(surfxml_bufferstack_stack, &surfxml_bufferstack);
    surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
  }
}

void surfxml_bufferstack_pop(int new_one)
{
  if (!new_one)
    surfxml_bufferstack = old_buff;
  else {
    free(surfxml_bufferstack);
    xbt_dynar_pop(surfxml_bufferstack_stack, &surfxml_bufferstack);
  }
}

/*
 * Trace related stuff
 */

xbt_dict_t traces_set_list = NULL;
XBT_PRIVATE xbt_dict_t trace_connect_list_host_avail = NULL;
XBT_PRIVATE xbt_dict_t trace_connect_list_host_speed = NULL;
XBT_PRIVATE xbt_dict_t trace_connect_list_link_avail = NULL;
XBT_PRIVATE xbt_dict_t trace_connect_list_link_bw = NULL;
XBT_PRIVATE xbt_dict_t trace_connect_list_link_lat = NULL;

void sg_platf_trace_connect(sg_platf_trace_connect_cbarg_t trace_connect)
{
  xbt_assert(xbt_dict_get_or_null(traces_set_list, trace_connect->trace),
              "Cannot connect trace %s to %s: trace unknown",
              trace_connect->trace,
              trace_connect->element);

  switch (trace_connect->kind) {
  case SURF_TRACE_CONNECT_KIND_HOST_AVAIL:
    xbt_dict_set(trace_connect_list_host_avail,
        trace_connect->trace,
        xbt_strdup(trace_connect->element), NULL);
    break;
  case SURF_TRACE_CONNECT_KIND_SPEED:
    xbt_dict_set(trace_connect_list_host_speed, trace_connect->trace,
        xbt_strdup(trace_connect->element), NULL);
    break;
  case SURF_TRACE_CONNECT_KIND_LINK_AVAIL:
    xbt_dict_set(trace_connect_list_link_avail,
        trace_connect->trace,
        xbt_strdup(trace_connect->element), NULL);
    break;
  case SURF_TRACE_CONNECT_KIND_BANDWIDTH:
    xbt_dict_set(trace_connect_list_link_bw,
        trace_connect->trace,
        xbt_strdup(trace_connect->element), NULL);
    break;
  case SURF_TRACE_CONNECT_KIND_LATENCY:
    xbt_dict_set(trace_connect_list_link_lat, trace_connect->trace,
        xbt_strdup(trace_connect->element), NULL);
    break;
  default:
  surf_parse_error("Cannot connect trace %s to %s: kind of trace unknown",
        trace_connect->trace, trace_connect->element);
    break;
  }
}


/* ***************************************** */

static int after_config_done;
void parse_after_config() {
  if (!after_config_done) {
    TRACE_start();

    /* Register classical callbacks */
    storage_register_callbacks();
    routing_register_callbacks();

    after_config_done = 1;
  }
}

/* This function acts as a main in the parsing area. */
void parse_platform_file(const char *file)
{
#if HAVE_LUA
  int is_lua = (file != NULL && strlen(file) > 3 && file[strlen(file)-3] == 'l' && file[strlen(file)-2] == 'u'
        && file[strlen(file)-1] == 'a');
#endif

  sg_platf_init();

#if HAVE_LUA
  /* Check if file extension is "lua". If so, we will use
   * the lua bindings to parse the platform file (since it is
   * written in lua). If not, we will use the (old?) XML parser
   */
  if (is_lua) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    luaL_loadfile(L, file); // This loads the file without executing it.

    /* Run the script */
    if (lua_pcall(L, 0, 0, 0)) {
        XBT_ERROR("FATAL ERROR:\n  %s: %s\n\n", "Lua call failed. Errormessage:", lua_tostring(L, -1));
        xbt_die("Lua call failed. See Log");
    }
  }
  else
#endif
  { // Use XML parser

    int parse_status;

    /* init the flex parser */
    surfxml_buffer_stack_stack_ptr = 1;
    surfxml_buffer_stack_stack[0] = 0;
    after_config_done = 0;
    surf_parse_open(file);

    traces_set_list = xbt_dict_new_homogeneous(NULL);
    trace_connect_list_host_avail = xbt_dict_new_homogeneous(free);
    trace_connect_list_host_speed = xbt_dict_new_homogeneous(free);
    trace_connect_list_link_avail = xbt_dict_new_homogeneous(free);
    trace_connect_list_link_bw = xbt_dict_new_homogeneous(free);
    trace_connect_list_link_lat = xbt_dict_new_homogeneous(free);

    /* Init my data */
    if (!surfxml_bufferstack_stack)
      surfxml_bufferstack_stack = xbt_dynar_new(sizeof(char *), NULL);

    /* Do the actual parsing */
    parse_status = surf_parse();

    /* connect all traces relative to hosts */
    xbt_dict_cursor_t cursor = NULL;
    char *trace_name, *elm;

    xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
      tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
      xbt_assert(trace, "Trace %s undefined", trace_name);

      simgrid::s4u::Host *host = sg_host_by_name(elm);
      xbt_assert(host, "Host %s undefined", elm);
      simgrid::surf::Cpu *cpu = host->pimpl_cpu;

      cpu->setStateTrace(trace);
    }
    xbt_dict_foreach(trace_connect_list_host_speed, cursor, trace_name, elm) {
      tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
      xbt_assert(trace, "Trace %s undefined", trace_name);

      simgrid::s4u::Host *host = sg_host_by_name(elm);
      xbt_assert(host, "Host %s undefined", elm);
      simgrid::surf::Cpu *cpu = host->pimpl_cpu;

      cpu->setSpeedTrace(trace);
    }
    xbt_dict_foreach(trace_connect_list_link_avail, cursor, trace_name, elm) {
      tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
      xbt_assert(trace, "Trace %s undefined", trace_name);
      Link *link = Link::byName(elm);
      xbt_assert(link, "Link %s undefined", elm);
      link->setStateTrace(trace);
    }

    xbt_dict_foreach(trace_connect_list_link_bw, cursor, trace_name, elm) {
      tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
      xbt_assert(trace, "Trace %s undefined", trace_name);
      Link *link = Link::byName(elm);
      xbt_assert(link, "Link %s undefined", elm);
      link->setBandwidthTrace(trace);
    }

    xbt_dict_foreach(trace_connect_list_link_lat, cursor, trace_name, elm) {
      tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
      xbt_assert(trace, "Trace %s undefined", trace_name);
      Link *link = Link::byName(elm);
      xbt_assert(link, "Link %s undefined", elm);
      link->setLatencyTrace(trace);
    }

    /* Free my data */
    xbt_dict_free(&trace_connect_list_host_avail);
    xbt_dict_free(&trace_connect_list_host_speed);
    xbt_dict_free(&trace_connect_list_link_avail);
    xbt_dict_free(&trace_connect_list_link_bw);
    xbt_dict_free(&trace_connect_list_link_lat);
    xbt_dict_free(&traces_set_list);
    xbt_dynar_free(&surfxml_bufferstack_stack);

    surf_parse_close();

    if (parse_status)
      surf_parse_error("Parse error in %s", file);

  }


}
