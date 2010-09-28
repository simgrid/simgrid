/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <ctype.h>

#include "surf_private.h"
#include "xbt/module.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_kernel, surf,
                                "Logging specific to SURF (kernel)");

/* Additional declarations for Windows potability. */

#ifndef MAX_DRIVE
#define MAX_DRIVE 26
#endif

#ifdef _XBT_WIN32
#include <windows.h>
static const char *disk_drives_letter_table[MAX_DRIVE] = {
  "A:\\",
  "B:\\",
  "C:\\",
  "D:\\",
  "E:\\",
  "F:\\",
  "G:\\",
  "H:\\",
  "I:\\",
  "J:\\",
  "K:\\",
  "L:\\",
  "M:\\",
  "N:\\",
  "O:\\",
  "P:\\",
  "Q:\\",
  "R:\\",
  "S:\\",
  "T:\\",
  "U:\\",
  "V:\\",
  "W:\\",
  "X:\\",
  "Y:\\",
  "Z:\\"
};
#endif /* #ifdef _XBT_WIN32 */

/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */

const char *__surf_get_initial_path(void)
{

#ifdef _XBT_WIN32
  unsigned i;
  char current_directory[MAX_PATH + 1] = { 0 };
  unsigned int len = GetCurrentDirectory(MAX_PATH + 1, current_directory);
  char root[4] = { 0 };

  if (!len)
    return NULL;

  strncpy(root, current_directory, 3);

  for (i = 0; i < MAX_DRIVE; i++) {
    if (toupper(root[0]) == disk_drives_letter_table[i][0])
      return disk_drives_letter_table[i];
  }

  return NULL;
#else
  return "./";
#endif
}

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path)
{
#ifdef _XBT_WIN32
  WIN32_FIND_DATA wfd = { 0 };
  HANDLE hFile = FindFirstFile(file_path, &wfd);

  if (INVALID_HANDLE_VALUE == hFile)
    return 0;

  FindClose(hFile);
  return 1;
#else
  return (file_path[0] == '/');
#endif
}

double NOW = 0;

xbt_dynar_t model_list = NULL;
tmgr_history_t history = NULL;
lmm_system_t maxmin_system = NULL;
xbt_dynar_t surf_path = NULL;

/* Don't forget to update the option description in smx_config when you change this */
s_surf_model_description_t surf_network_model_description[] = {
  {"Constant", "Simplistic network model where all communication take a constant time (one second)", NULL, surf_network_model_init_Constant},
  {"Vivaldi", "Scalable network model using the Vivaldi coordinate ideas", NULL, surf_network_model_init_Vivaldi},
  {"CM02", "Realistic network model with lmm_solve and no correction factors", NULL, surf_network_model_init_CM02},
  {"LV08", "Realistic network model with lmm_solve and these correction factors: latency*=10.4, bandwidth*=.92, S=8775" , NULL, surf_network_model_init_LegrandVelho},
  {"SMPI", "Realistic network model with lmm_solve and correction factors on three intervals (< 1KiB, < 64 KiB, >= 64 KiB)", NULL, surf_network_model_init_SMPI},
#ifdef HAVE_GTNETS
  {"GTNets", "Network Pseudo-model using the GTNets simulator instead of an analytic model", NULL, surf_network_model_init_GTNETS},
#endif
  {"Reno", "Model using lagrange_solve instead of lmm_solve (experts only)", NULL, surf_network_model_init_Reno},
  {"Reno2", "Model using lagrange_solve instead of lmm_solve (experts only)", NULL, surf_network_model_init_Reno2},
  {"Vegas", "Model using lagrange_solve instead of lmm_solve (experts only)", NULL, surf_network_model_init_Vegas},
  {NULL, NULL, NULL, NULL}            /* this array must be NULL terminated */
};

s_surf_model_description_t surf_cpu_model_description[] = {
  {"Cas01_fullupdate", "CPU classical model time=size/power", NULL, surf_cpu_model_init_Cas01},
  {"Cas01", "Variation of Cas01_fullupdate with partial invalidation optimization of lmm system. Should produce the same values, only faster", NULL, surf_cpu_model_init_Cas01_im},
  {"CpuTI", "Variation of Cas01 with also trace integration. Should produce the same values, only faster if you use availability traces", NULL, surf_cpu_model_init_ti},
  {NULL, NULL, NULL, NULL}            /* this array must be NULL terminated */
};

s_surf_model_description_t surf_workstation_model_description[] = {
  {"CLM03", "Default workstation model, using LV08 and CM02 as network and CPU", NULL, surf_workstation_model_init_CLM03, create_workstations},
  {"compound", "Workstation model allowing you to use other network and CPU models", NULL, surf_workstation_model_init_compound, create_workstations},
  {"ptask_L07", "Workstation model with better parallel task modeling", NULL, surf_workstation_model_init_ptask_L07, NULL},
  {NULL, NULL, NULL, NULL}            /* this array must be NULL terminated */
};

void update_model_description(s_surf_model_description_t * table,
                              const char *name, surf_model_t model)
{
  int i = find_model_description(table, name);
  table[i].model = model;
}

/** Displays the long description of all registered models, and quit */
void model_help(const char* category, s_surf_model_description_t * table) {
  int i;
  printf("Long description of the %s models accepted by this simulator:\n",category);
  for (i = 0; table[i].name; i++)
    printf("  %s: %s\n", table[i].name, table[i].description);
}

int find_model_description(s_surf_model_description_t * table,
                           const char *name)
{
  int i;
  char *name_list = NULL;

  for (i = 0; table[i].name; i++)
    if (!strcmp(name, table[i].name)) {
      return i;
    }
  name_list = strdup(table[0].name);
  for (i = 1; table[i].name; i++) {
    name_list =
      xbt_realloc(name_list, strlen(name_list) + strlen(table[i].name) + 2);
    strcat(name_list, ", ");
    strcat(name_list, table[i].name);
  }
  xbt_assert2(0, "Model '%s' is invalid! Valid models are: %s.", name,
              name_list);
}

double generic_maxmin_share_resources(xbt_swag_t running_actions,
                                      size_t offset,
                                      lmm_system_t sys,
                                      void (*solve) (lmm_system_t))
{
  surf_action_t action = NULL;
  double min = -1;
  double value = -1;
#define VARIABLE(action) (*((lmm_variable_t*)(((char *) (action)) + (offset))))

  xbt_assert0(solve, "Give me a real solver function!");
  solve(sys);

  xbt_swag_foreach(action, running_actions) {
    value = lmm_variable_getvalue(VARIABLE(action));
    if ((value > 0) || (action->max_duration >= 0))
      break;
  }

  if (!action)
    return -1.0;

  if (value > 0) {
    if (action->remains > 0)
      min = action->remains / value;
    else
      min = 0.0;
    if ((action->max_duration >= 0) && (action->max_duration < min))
      min = action->max_duration;
  } else
    min = action->max_duration;


  for (action = xbt_swag_getNext(action, running_actions->offset);
       action; action = xbt_swag_getNext(action, running_actions->offset)) {
    value = lmm_variable_getvalue(VARIABLE(action));
    if (value > 0) {
      if (action->remains > 0)
        value = action->remains / value;
      else
        value = 0.0;
      if (value < min) {
        min = value;
        DEBUG2("Updating min (value) with %p: %f", action, min);
      }
    }
    if ((action->max_duration >= 0) && (action->max_duration < min)) {
      min = action->max_duration;
      DEBUG2("Updating min (duration) with %p: %f", action, min);
    }
  }
  DEBUG1("min value : %f", min);

#undef VARIABLE
  return min;
}

XBT_LOG_EXTERNAL_CATEGORY(surf_cpu);
XBT_LOG_EXTERNAL_CATEGORY(surf_kernel);
XBT_LOG_EXTERNAL_CATEGORY(surf_lagrange);
XBT_LOG_EXTERNAL_CATEGORY(surf_lagrange_dichotomy);
XBT_LOG_EXTERNAL_CATEGORY(surf_maxmin);
XBT_LOG_EXTERNAL_CATEGORY(surf_network);
XBT_LOG_EXTERNAL_CATEGORY(surf_trace);
XBT_LOG_EXTERNAL_CATEGORY(surf_parse);
XBT_LOG_EXTERNAL_CATEGORY(surf_timer);
XBT_LOG_EXTERNAL_CATEGORY(surf_workstation);
XBT_LOG_EXTERNAL_CATEGORY(surf_config);
XBT_LOG_EXTERNAL_CATEGORY(surf_route);

#ifdef HAVE_GTNETS
XBT_LOG_EXTERNAL_CATEGORY(surf_network_gtnets);
#endif

void surf_init(int *argc, char **argv)
{ 
  /* Connect our log channels: that must be done manually under windows */
  XBT_LOG_CONNECT(surf_cpu, surf);
  XBT_LOG_CONNECT(surf_kernel, surf);
  XBT_LOG_CONNECT(surf_lagrange, surf);
  XBT_LOG_CONNECT(surf_lagrange_dichotomy, surf_lagrange);
  XBT_LOG_CONNECT(surf_maxmin, surf);
  XBT_LOG_CONNECT(surf_network, surf);
  XBT_LOG_CONNECT(surf_trace, surf);
  XBT_LOG_CONNECT(surf_parse, surf);
  XBT_LOG_CONNECT(surf_timer, surf);
  XBT_LOG_CONNECT(surf_workstation, surf);
  XBT_LOG_CONNECT(surf_config, surf);
  XBT_LOG_CONNECT(surf_route, surf);

#ifdef HAVE_GTNETS
  XBT_LOG_CONNECT(surf_network_gtnets, surf);
#endif

  xbt_init(argc, argv);
  if (!model_list)
    model_list = xbt_dynar_new(sizeof(surf_model_private_t), NULL);
  if (!history)
    history = tmgr_history_new();

  surf_config_init(argc, argv);
#ifdef HAVE_MC
  if (_surf_do_model_check)
    MC_memory_init();
#endif
}

#ifdef _XBT_WIN32
# define FILE_DELIM "\\"
#else
# define FILE_DELIM "/"         /* FIXME: move to better location */
#endif

FILE *surf_fopen(const char *name, const char *mode)
{
  unsigned int cpt;
  char *path_elm = NULL;
  char *buff;
  FILE *file = NULL;

  xbt_assert(name);

  if (__surf_is_absolute_file_path(name))       /* don't mess with absolute file names */
    return fopen(name, mode);

  /* search relative files in the path */
  xbt_dynar_foreach(surf_path, cpt, path_elm) {
    buff = bprintf("%s" FILE_DELIM "%s", path_elm, name);
    file = fopen(buff, mode);
    free(buff);

    if (file)
      return file;
  }
  return NULL;
}

void surf_exit(void)
{
  unsigned int iter;
  surf_model_t model = NULL;

  surf_config_finalize();

  xbt_dynar_foreach(model_list, iter, model)
    model->model_private->finalize();
  xbt_dynar_free(&model_list);

  if (maxmin_system) {
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
  if (history) {
    tmgr_history_free(history);
    history = NULL;
  }

  if (surf_path)
    xbt_dynar_free(&surf_path);

  tmgr_finalize();
  surf_parse_lex_destroy();
  surf_parse_free_callbacks();
  NOW = 0;                      /* Just in case the user plans to restart the simulation afterward */
}

void surf_presolve(void)
{
  double next_event_date = -1.0;
  tmgr_trace_event_t event = NULL;
  double value = -1.0;
  surf_resource_t resource = NULL;
  surf_model_t model = NULL;
  unsigned int iter;

  DEBUG0
    ("First Run! Let's \"purge\" events and put models in the right state");
  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if (next_event_date > NOW)
      break;
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value, (void **) &resource))) {
      resource->model->model_private->update_resource_state(resource,
                                                            event, value,
                                                            NOW);
    }
  }
  xbt_dynar_foreach(model_list, iter, model)
    model->model_private->update_actions_state(NOW, 0.0);
}

double surf_solve(void)
{
  double min = -1.0;
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  surf_resource_t resource = NULL;
  surf_model_t model = NULL;
  tmgr_trace_event_t event = NULL;
  unsigned int iter;

  min = -1.0;

  DEBUG0("Looking for next action end");
  xbt_dynar_foreach(model_list, iter, model) {
    DEBUG1("Running for Resource [%s]", model->name);
    model_next_action_end = model->model_private->share_resources(NOW);
    DEBUG2("Resource [%s] : next action end = %f",
           model->name, model_next_action_end);
    if (((min < 0.0) || (model_next_action_end < min))
        && (model_next_action_end >= 0.0))
      min = model_next_action_end;
  }
  DEBUG1("Next action end : %f", min);

  DEBUG0("Looking for next event");
  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    DEBUG1("Next TRACE event : %f", next_event_date);
    if ((min != -1.0) && (next_event_date > NOW + min))
      break;
    DEBUG0("Updating models");
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value, (void **) &resource))) {
      if (resource->model->model_private->resource_used(resource)) {
        min = next_event_date - NOW;
        DEBUG1
          ("This event will modify model state. Next event set to %f", min);
      }
      /* update state of model_obj according to new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      DEBUG2("Calling update_resource_state for resource %s with min %lf",
             resource->model->name, min);
      resource->model->model_private->update_resource_state(resource, event,
                                                            value, NOW + min);
    }
  }

  /* FIXME: Moved this test to here to avoid stoping simulation if there are actions running on cpus and all cpus are with availability = 0. 
   * This may cause an infinite loop if one cpu has a trace with periodicity = 0 and the other a trace with periodicity > 0.
   * The options are: all traces with same periodicity(0 or >0) or we need to change the way how the events are managed */
  if (min < 0.0)
    return -1.0;

  DEBUG1("Duration set to %f", min);

  NOW = NOW + min;

  xbt_dynar_foreach(model_list, iter, model)
    model->model_private->update_actions_state(NOW, min);

  return min;
}

double surf_get_clock(void)
{
  return NOW;
}
