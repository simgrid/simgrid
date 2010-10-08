/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

#define VARIABLE_SEPARATOR '#'

//to check if variables were previously set to 0, otherwise paje won't simulate them
static xbt_dict_t platform_variables; /* host or link name -> array of categories */

//B
static xbt_dict_t method_b_dict;

//C
static xbt_dict_t method_c_dict;

/* auxiliary function for resource utilization tracing */
static char *strsplit (char *input, int field, char del) //caller should free the returned string
{
  char *ret = NULL;
  int length = strlen(input), i;
  int s = 0, e = length+1;
  int current_field = 0;
  for (i = 0; i < length; i++){
     if (input[i] == del){
       if (current_field == field){
         e = i-1;
         break;
       }else{
         s = i+1;
         current_field++;
       }
     }
  }
  //copy string from s to e (with length equal to e-s) and return
  ret = malloc ((e-s+2)*sizeof(char));
  strncpy (ret, input+s, e-s+1);
  ret[e-s+1] = '\0';
  return ret;
}

//resource utilization tracing method
static void (*TRACE_method_alloc)(void) = NULL;
static void (*TRACE_method_release)(void) = NULL;
static void (*TRACE_method_start)(smx_action_t action) = NULL;
static void (*TRACE_method_event)(smx_action_t action, double now, double delta, const char *variable, const char *resource, double value) = NULL;
static void (*TRACE_method_end)(smx_action_t action) = NULL;

//used by all methods
static void __TRACE_surf_check_variable_set_to_zero (double now, const char *variable, const char *resource)
{
  /* check if we have to set it to 0 */
  if (!xbt_dict_get_or_null (platform_variables, resource)){
    xbt_dynar_t array = xbt_dynar_new(sizeof(char*), xbt_free);
    char *var_cpy = xbt_strdup(variable);
    xbt_dynar_push (array, &var_cpy);
    if (IS_TRACING_PLATFORM) pajeSetVariable (now, variable, resource, "0");
    xbt_dict_set (platform_variables, resource, array, xbt_dynar_free_voidp);
  }else{
    xbt_dynar_t array = xbt_dict_get (platform_variables, resource);
    unsigned int i;
    char* cat;
    int flag = 0;
    xbt_dynar_foreach (array, i, cat) {
      if (strcmp(variable, cat)==0){
        flag = 1;
      }
    }
    if (flag==0){
      char *var_cpy = xbt_strdup(variable);
      xbt_dynar_push (array, &var_cpy);
      if (IS_TRACING_PLATFORM) pajeSetVariable (now, variable, resource, "0");
    }
  }
  /* end of check */
}

#define A_METHOD
//A
static void __TRACE_A_alloc (void) {}
static void __TRACE_A_release (void) {}
static void __TRACE_A_start (smx_action_t action) {}
static void __TRACE_A_event (smx_action_t action, double now, double delta, const char *variable, const char *resource, double value)
{
  if (!IS_TRACING_PLATFORM) return;

  char valuestr[100];
  snprintf (valuestr, 100, "%f", value);

  __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
  pajeAddVariable (now, variable, resource, valuestr);
  pajeSubVariable (now+delta, variable, resource, valuestr);
}
static void __TRACE_A_end (smx_action_t action) {}

#define B_METHOD
//B

static void __TRACE_B_alloc (void)
{
  method_b_dict =  xbt_dict_new();
}

static void __TRACE_B_release (void)
{
  xbt_dict_cursor_t cursor = NULL;
  unsigned int cursor_ar = 0;
    char *key, *value, *res;
    char *resource;
    char *aux = NULL;
    char *var_cpy = NULL;
    xbt_dynar_t resources = NULL;
  if (!IS_TRACING_PLATFORM) return;
  if (!xbt_dict_length(method_b_dict)){
    return;
  }else{
    /* get all resources from method_b_dict */
    resources = xbt_dynar_new(sizeof(char*), xbt_free);
    xbt_dict_foreach(method_b_dict, cursor, key, value) {
      res = strsplit (key, 0, VARIABLE_SEPARATOR);
      aux = strsplit (key, 1, VARIABLE_SEPARATOR);
      if (strcmp (aux, "Time") == 0){ //only need to add one of three
        var_cpy = xbt_strdup (res);
        xbt_dynar_push (resources, &var_cpy);
      }
      free (aux);
      free (res);
    }

    /* iterate through resources array */
    xbt_dynar_foreach (resources, cursor_ar, resource) {
      char timekey[100], valuekey[100], variablekey[100];
      char *time = NULL;
      char *value = NULL;
      char *variable = NULL;
      snprintf (timekey, 100, "%s%cTime", resource, VARIABLE_SEPARATOR);
      snprintf (valuekey, 100, "%s%cValue", resource, VARIABLE_SEPARATOR);
      snprintf (variablekey, 100, "%s%cVariable", resource, VARIABLE_SEPARATOR);

      time = xbt_dict_get_or_null (method_b_dict, timekey);
      if (!time) continue;
      value = xbt_dict_get (method_b_dict, valuekey);
      variable = xbt_dict_get (method_b_dict, variablekey);
      pajeSubVariable (atof(time), variable, resource, value);

      xbt_dict_remove (method_b_dict, timekey);
      xbt_dict_remove (method_b_dict, valuekey);
      xbt_dict_remove (method_b_dict, variablekey);
    }
  }
  xbt_dict_free (&method_b_dict);
}

static void __TRACE_B_start (smx_action_t action) {}
static void __TRACE_B_event (smx_action_t action, double now, double delta, const char *variable, const char *resource, double value)
{

  if (!IS_TRACING_PLATFORM) return;

  char valuestr[100];
  char nowstr[100], nowdeltastr[100];
  char timekey[100], valuekey[100], variablekey[100];
  char *lastvariable = NULL;
  char *lasttime = NULL;
  char *lastvalue = NULL;
  char *nowdeltastr_cpy = NULL;
  char *valuestr_cpy = NULL;
  char *variable_cpy = NULL;

  /*
   * The following code replaces the code above with the objective
   * to decrease the size of file because of unnecessary add/sub on
   * variables. It should be re-checked before put in production.
   */

  snprintf (valuestr, 100, "%f", value);
  snprintf (nowstr, 100, "%f", now);
  snprintf (nowdeltastr, 100, "%f", now+delta);
  snprintf (timekey, 100, "%s%cTime", resource, VARIABLE_SEPARATOR);
  snprintf (valuekey, 100, "%s%cValue", resource, VARIABLE_SEPARATOR);
  snprintf (variablekey, 100, "%s%cVariable", resource, VARIABLE_SEPARATOR);

  lastvariable = xbt_dict_get_or_null (method_b_dict, variablekey);
  if (lastvariable == NULL){
    __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
    pajeAddVariable (now, variable, resource, valuestr);
    nowdeltastr_cpy = xbt_strdup (nowdeltastr);
    valuestr_cpy = xbt_strdup (valuestr);
    variable_cpy = xbt_strdup (variable);
    xbt_dict_set (method_b_dict, timekey, nowdeltastr_cpy, xbt_free);
    xbt_dict_set (method_b_dict, valuekey, valuestr_cpy, xbt_free);
    xbt_dict_set (method_b_dict, variablekey, variable_cpy, xbt_free);
  }else{
    lasttime = xbt_dict_get_or_null (method_b_dict, timekey);
    lastvalue = xbt_dict_get_or_null (method_b_dict, valuekey);

    /* check if it is the same variable */
    if (strcmp(lastvariable, variable) == 0){ /* same variable */
      /* check if lasttime equals now */
      if (atof(lasttime) == now){ /* lastime == now */
        /* check if lastvalue equals valuestr */
        if (atof(lastvalue) == value){ /* lastvalue == value (good, just advance time) */
          char *nowdeltastr_cpy = xbt_strdup (nowdeltastr);
          xbt_dict_set (method_b_dict, timekey, nowdeltastr_cpy, xbt_free);
        }else{ /* value has changed */
          /* value has changed, subtract previous value, add new one */
          pajeSubVariable (atof(lasttime), variable, resource, lastvalue);
          pajeAddVariable (atof(nowstr), variable, resource, valuestr);
          nowdeltastr_cpy = xbt_strdup (nowdeltastr);
          valuestr_cpy = xbt_strdup (valuestr);
          xbt_dict_set (method_b_dict, timekey, nowdeltastr_cpy, xbt_free);
          xbt_dict_set (method_b_dict, valuekey, valuestr_cpy, xbt_free);
        }
      }else{ /* lasttime != now */
        /* the last time is different from new starting time, subtract to lasttime and add from nowstr */
        pajeSubVariable (atof(lasttime), variable, resource, lastvalue);
        pajeAddVariable (atof(nowstr), variable, resource, valuestr);
        nowdeltastr_cpy = xbt_strdup (nowdeltastr);
        valuestr_cpy = xbt_strdup (valuestr);
        xbt_dict_set (method_b_dict, timekey, nowdeltastr_cpy, xbt_free);
        xbt_dict_set (method_b_dict, valuekey, valuestr_cpy, xbt_free);
      }
    }else{ /* variable has changed */
      pajeSubVariable (atof(lasttime), lastvariable, resource, lastvalue);
      __TRACE_surf_check_variable_set_to_zero (now, variable, resource);
      pajeAddVariable (now, variable, resource, valuestr);
      nowdeltastr_cpy = xbt_strdup (nowdeltastr);
      valuestr_cpy = xbt_strdup (valuestr);
      variable_cpy = xbt_strdup (variable);
      xbt_dict_set (method_b_dict, timekey, nowdeltastr_cpy, xbt_free);
      xbt_dict_set (method_b_dict, valuekey, valuestr_cpy, xbt_free);
      xbt_dict_set (method_b_dict, variablekey, variable_cpy, xbt_free);
    }
  }
  return;
}
static void __TRACE_B_end (smx_action_t action) {}

#define C_METHOD
//C
static void __TRACE_C_alloc (void)
{
  method_c_dict = xbt_dict_new();
}

static void __TRACE_C_release (void)
{
  xbt_dict_free (&method_c_dict);
}

static void __TRACE_C_start (smx_action_t action)
{
  char key[100];
  snprintf (key, 100, "%p", action);

  //check if exists
  if (xbt_dict_get_or_null (method_c_dict, key)){
    xbt_dict_remove (method_c_dict, key); //should never execute here, but it does
  }
  xbt_dict_set (method_c_dict, key, xbt_dict_new(), xbt_free);
}

static void __TRACE_C_event (smx_action_t action, double now, double delta, const char *variable, const char *resource, double value)
{
  char key[100];
  snprintf (key, 100, "%p", action);

  xbt_dict_t action_dict = xbt_dict_get (method_c_dict, key);
  //setting start time
  if (!xbt_dict_get_or_null (action_dict, "start")){
    char start_time[100];
    snprintf (start_time, 100, "%f", now);
    xbt_dict_set (action_dict, "start", xbt_strdup (start_time), xbt_free);
  }
  //updating end time
  char end_time[100];
  snprintf (end_time, 100, "%f", now+delta);
  xbt_dict_set (action_dict, "end", xbt_strdup (end_time), xbt_free);

  //accumulate the value resource-variable
  char res_var[300];
  snprintf (res_var, 300, "%s %s", resource, variable);
  double current_value_f;
  char *current_value = xbt_dict_get_or_null (action_dict, res_var);
  if (current_value){
    current_value_f = atof (current_value);
    current_value_f += value*delta;
  }else{
    current_value_f = value*delta;
  }
  char new_current_value[100];
  snprintf (new_current_value, 100, "%f", current_value_f);
  xbt_dict_set (action_dict, res_var, xbt_strdup (new_current_value), xbt_free);
}

static void __TRACE_C_end (smx_action_t action)
{
  char key[100];
  snprintf (key, 100, "%p", action);

  xbt_dict_t action_dict = xbt_dict_get (method_c_dict, key);
  double start_time = atof(xbt_dict_get (action_dict, "start"));
  double end_time = atof(xbt_dict_get (action_dict, "end"));

  xbt_dict_cursor_t cursor=NULL;
  char *action_dict_key, *action_dict_value;
  xbt_dict_foreach(action_dict,cursor,action_dict_key,action_dict_value) {
    char resource[100], variable[100];
    if (sscanf (action_dict_key, "%s %s", resource, variable) != 2) continue;
    __TRACE_surf_check_variable_set_to_zero (start_time, variable, resource);
    char value_str[100];
    if(end_time-start_time != 0){
      snprintf (value_str, 100, "%f", atof(action_dict_value)/(end_time-start_time));
      pajeAddVariable (start_time, variable, resource, value_str);
      pajeSubVariable (end_time, variable, resource, value_str);
    }
  }
  xbt_dict_remove (method_c_dict, key);
}

#define RESOURCE_UTILIZATION_INTERFACE
/*
 * TRACE_surf_link_set_utilization: entry point from SimGrid
 */
void TRACE_surf_link_set_utilization (void *link, smx_action_t smx_action, surf_action_t surf_action, double value, double now, double delta)
{
  if (!IS_TRACING) return;
  if (!value) return;
  //only trace link utilization if link is known by tracing mechanism
  if (!TRACE_surf_link_is_traced (link)) return;
  if (!value) return;

  //trace uncategorized link utilization
  char resource[100];
  snprintf (resource, 100, "%p", link);
  TRACE_surf_resource_utilization_event (smx_action, now, delta, "bandwidth_used", resource, value);

  //trace categorized utilization
  if (!IS_TRACED(surf_action)) return;
  char type[100];
  snprintf (type, 100, "b%s", surf_action->category);
  TRACE_surf_resource_utilization_event (smx_action, now, delta, type, resource, value);
  return;
}

/*
 * TRACE_surf_host_set_utilization: entry point from SimGrid
 */
void TRACE_surf_host_set_utilization (const char *name, smx_action_t smx_action, surf_action_t surf_action, double value, double now, double delta)
{
  if (!IS_TRACING) return;
  if (!value) return;

  //trace uncategorized host utilization
  TRACE_surf_resource_utilization_event (smx_action, now, delta, "power_used", name, value);

  //trace categorized utilization
  if (!IS_TRACED(surf_action)) return;
  char type[100];
  snprintf (type, 100, "p%s", surf_action->category);
  TRACE_surf_resource_utilization_event (smx_action, now, delta, type, name, value);
  return;
}

/*
 * __TRACE_surf_resource_utilization_*: entry points from tracing functions
 */
void TRACE_surf_resource_utilization_start (smx_action_t action)
{
  if (!IS_TRACING) return;
  TRACE_method_start (action);
}

void TRACE_surf_resource_utilization_event (smx_action_t action, double now, double delta, const char *variable, const char *resource, double value)
{
  if (!IS_TRACING) return;
  TRACE_method_event (action, now, delta, variable, resource, value);
}

void TRACE_surf_resource_utilization_end (smx_action_t action)
{
  if (!IS_TRACING) return;
  TRACE_method_end (action);
}

void TRACE_surf_resource_utilization_release ()
{
  if (!IS_TRACING) return;
  TRACE_method_release ();
}

static void __TRACE_define_method (char *method)
{
  if (!strcmp(method, "a")){
    TRACE_method_alloc = __TRACE_A_alloc;
    TRACE_method_release = __TRACE_A_release;
    TRACE_method_start = __TRACE_A_start;
    TRACE_method_event = __TRACE_A_event;
    TRACE_method_end = __TRACE_A_end;
  }else if (!strcmp(method, "c")){
    TRACE_method_alloc = __TRACE_C_alloc;
    TRACE_method_release = __TRACE_C_release;
    TRACE_method_start = __TRACE_C_start;
    TRACE_method_event = __TRACE_C_event;
    TRACE_method_end = __TRACE_C_end;
  }else{ //default is B
    TRACE_method_alloc = __TRACE_B_alloc;
    TRACE_method_release = __TRACE_B_release;
    TRACE_method_start = __TRACE_B_start;
    TRACE_method_event = __TRACE_B_event;
    TRACE_method_end = __TRACE_B_end;
  }
}

void TRACE_surf_resource_utilization_alloc ()
{
  platform_variables = xbt_dict_new();
  __TRACE_define_method (TRACE_get_platform_method());
  TRACE_method_alloc ();
}
#endif
