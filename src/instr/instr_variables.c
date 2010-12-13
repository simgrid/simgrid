/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_private.h"
#include "surf/surf_private.h"
#include "surf/network_private.h"

#ifdef HAVE_TRACING

//extern routing_global_t global_routing;
extern xbt_dict_t hosts_types;
extern xbt_dict_t links_types;

void TRACE_user_link_variable(double time, const char *resource,
                              const char *variable,
                              double value, const char *what)
{
  if (!TRACE_is_active())
    return;

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  if (strcmp(what, "declare") == 0) {
    {
      //check if links have been created
      xbt_assert1 (links_types != NULL && xbt_dict_length(links_types) != 0,
          "%s must be called after environment creation", __FUNCTION__);
    }

    char new_type[INSTR_DEFAULT_STR_SIZE];
    xbt_dict_cursor_t cursor = NULL;
    char *type;
    void *data;
    xbt_dict_foreach(links_types, cursor, type, data) {
      snprintf (new_type, INSTR_DEFAULT_STR_SIZE, "%s-%s", variable, type);
      pajeDefineVariableType (new_type, type, variable);
    }
  } else{
    char *link_type = instr_link_type (resource);
    xbt_assert2 (link_type != NULL,
        "link %s provided to %s is not known by the tracing mechanism", resource, __FUNCTION__);
    char variable_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (variable_type, INSTR_DEFAULT_STR_SIZE, "%s-%s", variable, link_type);

    if (strcmp(what, "set") == 0) {
      pajeSetVariable(time, variable_type, resource, valuestr);
    } else if (strcmp(what, "add") == 0) {
      pajeAddVariable(time, variable_type, resource, valuestr);
    } else if (strcmp(what, "sub") == 0) {
      pajeSubVariable(time, variable_type, resource, valuestr);
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
    {
      //check if hosts have been created
      xbt_assert1 (hosts_types != NULL && xbt_dict_length(hosts_types) != 0,
          "%s must be called after environment creation", __FUNCTION__);
    }

    char new_type[INSTR_DEFAULT_STR_SIZE];
    xbt_dict_cursor_t cursor = NULL;
    char *type;
    void *data;
    xbt_dict_foreach(hosts_types, cursor, type, data) {
      snprintf (new_type, INSTR_DEFAULT_STR_SIZE, "%s-%s", variable, type);
      pajeDefineVariableType (new_type, type, variable);
    }
  } else{
    char *host_name = MSG_host_self()->name;
    char *host_type = instr_host_type (host_name);
    char variable_type[INSTR_DEFAULT_STR_SIZE];
    snprintf (variable_type, INSTR_DEFAULT_STR_SIZE, "%s-%s", variable, host_type);

    if (strcmp(what, "set") == 0) {
      pajeSetVariable(time, variable_type, host_name, valuestr);
    } else if (strcmp(what, "add") == 0) {
      pajeAddVariable(time, variable_type, host_name, valuestr);
    } else if (strcmp(what, "sub") == 0) {
      pajeSubVariable(time, variable_type, host_name, valuestr);
    }
  }
}

#endif /* HAVE_TRACING */
