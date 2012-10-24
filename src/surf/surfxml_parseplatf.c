/* Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
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

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

/*
 *  Allow the cluster tag to mess with the parsing buffer.
 * (this will probably become obsolete once the cluster tag do not mess with the parsing callbacks directly)
 */

/* This buffer is used to store the original buffer before substituting it by out own buffer. Useful for the cluster tag */
static xbt_dynar_t surfxml_bufferstack_stack = NULL;
int surfxml_bufferstack_size = 2048;

static char *old_buff = NULL;

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


/* This function acts as a main in the parsing area. */
void parse_platform_file(const char *file)
{
  int parse_status;

  surf_parse_init_callbacks();

  /* Register classical callbacks */
  storage_register_callbacks();
  routing_register_callbacks();

  /* ***************************************** */
  /* TUTORIAL: New TAG                         */
  gpu_register_callbacks();
  /* ***************************************** */

  /* init the flex parser */
  surfxml_buffer_stack_stack_ptr = 1;
  surfxml_buffer_stack_stack[0] = 0;

  surf_parse_open(file);

  /* Init my data */
  if (!surfxml_bufferstack_stack)
    surfxml_bufferstack_stack = xbt_dynar_new(sizeof(char *), NULL);

  traces_set_list = xbt_dict_new_homogeneous(NULL);
  trace_connect_list_host_avail = xbt_dict_new_homogeneous(free);
  trace_connect_list_power = xbt_dict_new_homogeneous(free);
  trace_connect_list_link_avail = xbt_dict_new_homogeneous(free);
  trace_connect_list_bandwidth = xbt_dict_new_homogeneous(free);
  trace_connect_list_latency = xbt_dict_new_homogeneous(free);

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

  /* Stop the flex parser */
  surf_parse_close();
  if (parse_status)
    xbt_die("Parse error in %s", file);
}

