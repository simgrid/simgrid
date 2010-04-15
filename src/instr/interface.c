/*
 * interface.c
 *
 *  Created on: Nov 23, 2009
 *      Author: Lucas Schnorr
 *     License: This program is free software; you can redistribute
 *              it and/or modify it under the terms of the license
 *              (GNU LGPL) which comes with this package.
 *
 *     Copyright (c) 2009 The SimGrid team.
 */

#include "instr/private.h"


#ifdef HAVE_TRACING

XBT_LOG_NEW_DEFAULT_CATEGORY(tracing,"Tracing Interface");

static xbt_dict_t defined_types;
static xbt_dict_t created_categories;

int trace_mask;

/** \ingroup tracing
 * \brief Simple initialization of tracing.
 *
 * \param filename of the file that will contain the traces
 * \return 0 if everything is ok
 */
int TRACE_start (const char *filename)
{
  return TRACE_start_with_mask (filename, TRACE_PLATFORM);
}

/** \ingroup tracing
 * \brief Initialization of tracing.
 *
 * Function to be called at first when tracing a simulation
 *
 * \param name of the file that will contain the traces
 * \param mask to take into account during trace
 * \return 0 if everything is ok
 */
int TRACE_start_with_mask(const char *filename, int mask) {
  if (IS_TRACING) { /* what? trace is already active... ignore.. */
	THROW0 (tracing_error, TRACE_ERROR_START,
	        "TRACE_start called, but tracing is already active");
    return 0;
  }

  /* checking if the mask is good (only TRACE_PLATFORM for now) */
  if (mask != TRACE_PLATFORM){
    THROW0 (tracing_error, TRACE_ERROR_MASK,
            "Only TRACE_PLATFORM mask is accepted for now");
  }

  FILE *file = fopen(filename, "w");
  if (!file) {
    THROW1 (tracing_error, TRACE_ERROR_START,
    		"Tracefile %s could not be opened for writing.", filename);
  } else {
    TRACE_paje_start (file);
  }
  TRACE_paje_create_header();

  /* setting the mask */
  trace_mask = mask;

  //check if options are correct
  if (IS_TRACING_TASKS){
	if (!IS_TRACING_PROCESSES){
	  TRACE_end();
      THROW0 (tracing_error, TRACE_ERROR_MASK,
              "TRACE_PROCESS must be enabled if TRACE_TASK is used");
	}
  }

  if (IS_TRACING_PROCESSES|IS_TRACING_TASKS){
	if (!IS_TRACING_PLATFORM){
	  TRACE_end();
      THROW0 (tracing_error, TRACE_ERROR_MASK,
	          "TRACE_PLATFORM must be enabled if TRACE_PROCESS or TRACE_TASK is used");
	}
  }

  //defining platform hierarchy
  if (IS_TRACING_PLATFORM) pajeDefineContainerType("PLATFORM", "0", "platform");
  if (IS_TRACING_PLATFORM) pajeDefineContainerType("HOST", "PLATFORM", "HOST");
  if (IS_TRACING_PLATFORM) pajeDefineVariableType ("power", "HOST", "power");
  if (IS_TRACING_PLATFORM) pajeDefineContainerType("LINK", "PLATFORM", "LINK");
  if (IS_TRACING_PLATFORM) pajeDefineVariableType ("bandwidth", "LINK", "bandwidth");
  if (IS_TRACING_PLATFORM) pajeDefineVariableType ("latency", "LINK", "latency");

  if (IS_TRACING_PROCESSES) pajeDefineContainerType("PROCESS", "HOST", "PROCESS");
  if (IS_TRACING_PROCESSES) pajeDefineStateType("presence", "PROCESS", "presence");

  if (IS_TRACING_PROCESSES){
	if (IS_TRACING_TASKS) pajeDefineContainerType("TASK", "PROCESS", "TASK");
  }else{
	if (IS_TRACING_TASKS) pajeDefineContainerType("TASK", "HOST", "TASK");
  }

  if (IS_TRACING_PLATFORM) pajeCreateContainer(MSG_get_clock(), "platform", "PLATFORM", "0", "simgrid-platform");

  defined_types = xbt_dict_new();
  created_categories = xbt_dict_new();
  __TRACE_msg_init();
  __TRACE_surf_init();
  __TRACE_msg_process_init ();

  return 0;
}

int TRACE_end() {
  if (!IS_TRACING) return 1;
  FILE *file = TRACE_paje_end();
  fclose (file);
  return 0;
}

int TRACE_category (const char *category)
{
  if (!IS_TRACING) return 1;
  static int first_time = 1;
  if (first_time){
	  TRACE_define_type ("user_type", "0", 1);
	  first_time = 0;
  }
  return TRACE_create_category (category, "user_type", "0");
}

void TRACE_define_type (const char *type,
		const char *parent_type, int final) {
  if (!IS_TRACING) return;

  //check if type is already defined
  if (xbt_dict_get_or_null (defined_types, type)){
    THROW1 (tracing_error, TRACE_ERROR_TYPE_ALREADY_DEFINED, "Type %s is already defined", type);
  }
  //check if parent_type is already defined
  if (strcmp(parent_type, "0") && !xbt_dict_get_or_null (defined_types, parent_type)) {
    THROW1 (tracing_error, TRACE_ERROR_TYPE_NOT_DEFINED, "Type (used as parent) %s is not defined", parent_type);
  }

  pajeDefineContainerType(type, parent_type, type);
  if (final) {
    //for m_process_t
    if (IS_TRACING_PROCESSES) pajeDefineContainerType ("process", type, "process");
    if (IS_TRACING_PROCESSES) pajeDefineStateType ("process-state", "process", "process-state");

    if (IS_TRACING_TASKS) pajeDefineContainerType ("task", type, "task");
    if (IS_TRACING_TASKS) pajeDefineStateType ("task-state", "task", "task-state");
  }
  xbt_dict_set (defined_types, type, xbt_strdup("1"), xbt_free);
}

int TRACE_create_category (const char *category,
		const char *type, const char *parent_category)
{
  if (!IS_TRACING) return;

  //check if type is defined
  if (!xbt_dict_get_or_null (defined_types, type)) {
	 THROW1 (tracing_error, TRACE_ERROR_TYPE_NOT_DEFINED, "Type %s is not defined", type);
	 return 1;
  }
  //check if parent_category exists
  if (strcmp(parent_category, "0") && !xbt_dict_get_or_null (created_categories, parent_category)){
     THROW1 (tracing_error, TRACE_ERROR_CATEGORY_NOT_DEFINED, "Category (used as parent) %s is not created", parent_category);
     return 1;
  }
  //check if category is created
  if (xbt_dict_get_or_null (created_categories, category)){
     return 1;
  }

  pajeCreateContainer(MSG_get_clock(), category, type, parent_category, category);

  /* for registering application categories on top of platform */
  char state[100];
  snprintf (state, 100, "b%s", category);
  if (IS_TRACING_PLATFORM) pajeDefineVariableType (state, "LINK", state);
  snprintf (state, 100, "p%s", category);
  if (IS_TRACING_PLATFORM) pajeDefineVariableType (state, "HOST", state);

  xbt_dict_set (created_categories, category, xbt_strdup("1"), xbt_free);
  return 0;
}

void TRACE_set_mask (int mask)
{
  if (mask != TRACE_PLATFORM){
     return;
  }else{
     trace_mask = mask;
  }
}

#endif /* HAVE_TRACING */
