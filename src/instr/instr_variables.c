/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include "surf/surf_private.h"
#include "surf/network_private.h"

#ifdef HAVE_TRACING

void TRACE_user_link_variable(double time, const char *resource,
                              const char *variable,
                              double value, const char *what)
{
  if (!TRACE_is_active())
    return;

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  if (strcmp(what, "declare") == 0) {
    instr_new_user_link_variable_type (variable, NULL);
  } else{
    char *variable_id = instr_variable_type(variable, resource);
    char *resource_id = instr_resource_type(resource);
    if (strcmp(what, "set") == 0) {
      pajeSetVariable(time, variable_id, resource_id, valuestr);
    } else if (strcmp(what, "add") == 0) {
      pajeAddVariable(time, variable_id, resource_id, valuestr);
    } else if (strcmp(what, "sub") == 0) {
      pajeSubVariable(time, variable_id, resource_id, valuestr);
    }
  }
}

void TRACE_user_host_variable(double time, const char *variable,
                              double value, const char *what)
{
  if (!TRACE_is_active())
    return;

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  if (strcmp(what, "declare") == 0) {
    instr_new_user_host_variable_type (variable, NULL);
  } else{
    char *host_name = MSG_host_self()->name;
    char *variable_id = instr_variable_type(variable, host_name);
    char *resource_id = instr_resource_type(host_name);
    if (strcmp(what, "set") == 0) {
      pajeSetVariable(time, variable_id, resource_id, valuestr);
    } else if (strcmp(what, "add") == 0) {
      pajeAddVariable(time, variable_id, resource_id, valuestr);
    } else if (strcmp(what, "sub") == 0) {
      pajeSubVariable(time, variable_id, resource_id, valuestr);
    }
  }
}

#endif /* HAVE_TRACING */
