/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "simgrid/platf.h"
#include "surf/surfxml_parse.h"
#include "surf/surf_private.h"

#ifdef HAVE_LUA
#include "bindings/lua/simgrid_lua.h"
#include "bindings/lua/lua_state_cloner.h"

#include <lua.h>                /* Always include this when calling Lua */
#include <lauxlib.h>            /* Always include this when calling Lua */
#include <lualib.h>             /* Prototype for luaL_openlibs(), */
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

/*
 * Trace related stuff
 */

xbt_dict_t traces_set_list = NULL;
xbt_dict_t trace_connect_list_host_avail = NULL;
xbt_dict_t trace_connect_list_power = NULL;
xbt_dict_t trace_connect_list_link_avail = NULL;
xbt_dict_t trace_connect_list_bandwidth = NULL;
xbt_dict_t trace_connect_list_latency = NULL;

/* ********************************************* */
/* TUTORIAL: New TAG                             */
/* This function should be in gpu.c              */
/* because sg_platf_gpu_add_cb take a staic fct  */
XBT_PUBLIC(void) gpu_register_callbacks(void){
  sg_platf_gpu_add_cb(NULL);
}
/* ***************************************** */

static int after_config_done;
void parse_after_config() {
  if (!after_config_done) {
	  TRACE_start();

    /* Register classical callbacks */
    storage_register_callbacks();
    routing_register_callbacks();
    gpu_register_callbacks();

    /* ***************************************** */
    /* TUTORIAL: New TAG                         */
    /* ***************************************** */
    after_config_done = 1;
  }
}

/* This function acts as a main in the parsing area. */
void parse_platform_file(const char *file)
{
#ifdef HAVE_LUA
  int is_lua = (file != NULL && strlen(file) > 3 && file[strlen(file)-3] == 'l' && file[strlen(file)-2] == 'u'
        && file[strlen(file)-1] == 'a');
#endif

  surf_parse_init_callbacks();

#ifdef HAVE_LUA
  /* Check if file extension is "lua". If so, we will use
   * the lua bindings to parse the platform file (since it is
   * written in lua). If not, we will use the (old?) XML parser
   */
  if (is_lua) {
    // Get maestro state. In case we're calling Lua from
    // C only, this will be NULL -- no Lua code has been
    // executed yet and hence, the SimGrid module has not
    // yet been loaded.
    // NOTE: After executing the lua_pcall() below,
    // sglua_get_maestro() will not be NULL, since the
    // SimGrid module was loaded!
    lua_State* L = sglua_get_maestro();

    // We may want to remove the task_copy_callback from
    // the SimGrid module if we're using C code only (this
    // callback is used for Lua-only code).
    int remove_callback = FALSE;
    if (L == NULL) {
        L = luaL_newstate();
        remove_callback = TRUE;
    }
    luaL_openlibs(L);

    luaL_loadfile(L, file); // This loads the file without executing it.

    /* Run the script */
    if (lua_pcall(L, 0, 0, 0)) {
        XBT_ERROR("FATAL ERROR:\n  %s: %s\n\n", "Lua call failed. Errormessage:", lua_tostring(L, -1));
        xbt_die("Lua call failed. See Log");
    }
    // Without this, task_copy_callback() will try to copy
    // some tasks -- but these don't exist in case we're using
    // C. Hence, we need to remove the callback -- we don't
    // want to segfault.
    if (remove_callback) {
      MSG_task_set_copy_callback(NULL);
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
    trace_connect_list_power = xbt_dict_new_homogeneous(free);
    trace_connect_list_link_avail = xbt_dict_new_homogeneous(free);
    trace_connect_list_bandwidth = xbt_dict_new_homogeneous(free);
    trace_connect_list_latency = xbt_dict_new_homogeneous(free);

    /* Init my data */
    if (!surfxml_bufferstack_stack)
      surfxml_bufferstack_stack = xbt_dynar_new(sizeof(char *), NULL);

    /* Do the actual parsing */
    parse_status = surf_parse();

    /* Free my data */
    xbt_dict_free(&trace_connect_list_host_avail);
    xbt_dict_free(&trace_connect_list_power);
    xbt_dict_free(&trace_connect_list_link_avail);
    xbt_dict_free(&trace_connect_list_bandwidth);
    xbt_dict_free(&trace_connect_list_latency);
    xbt_dict_free(&traces_set_list);
    xbt_dict_free(&random_data_list);
    xbt_dynar_free(&surfxml_bufferstack_stack);

    surf_parse_close();

    if (parse_status)
      surf_parse_error("Parse error in %s", file);

  }


}
