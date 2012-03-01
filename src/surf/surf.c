/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <ctype.h>

#include "surf_private.h"
#include "xbt/module.h"
#include "mc/mc.h"
#include "surf/surf_resource.h"
#include "xbt/xbt_os_thread.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_kernel, surf,
                                "Logging specific to SURF (kernel)");

/* Additional declarations for Windows portability. */

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
#endif                          /* #ifdef _XBT_WIN32 */

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
  {"LV08",
   "Realistic network analytic model (slow-start modeled by multiplying latency by 10.4, bandwidth by .92; bottleneck sharing uses a payload of S=8775 for evaluating RTT). ",
   surf_network_model_init_LegrandVelho},
  {"Constant",
   "Simplistic network model where all communication take a constant time (one second). This model provides the lowest realism, but is (marginally) faster.",
   surf_network_model_init_Constant},
  {"SMPI",
   "Realistic network model specifically tailored for HPC settings (accurate modeling of slow start with correction factors on three intervals: < 1KiB, < 64 KiB, >= 64 KiB)",
   surf_network_model_init_SMPI},
  {"CM02",
   "Legacy network analytic model (Very similar to LV08, but without corrective factors. The timings of small messages are thus poorly modeled).",
   surf_network_model_init_CM02},
#ifdef HAVE_GTNETS
  {"GTNets",
   "Network pseudo-model using the GTNets simulator instead of an analytic model",
   surf_network_model_init_GTNETS},
#endif
#ifdef HAVE_NS3
  {"NS3",
   "Network pseudo-model using the NS3 tcp model instead of an analytic model",
	surf_network_model_init_NS3},
#endif
  {"Reno",
   "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
   surf_network_model_init_Reno},
  {"Reno2",
   "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
   surf_network_model_init_Reno2},
  {"Vegas",
   "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
   surf_network_model_init_Vegas},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_cpu_model_description[] = {
  {"Cas01",
   "Simplistic CPU model (time=size/power).",
   surf_cpu_model_init_Cas01},
  {NULL, NULL,  NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_workstation_model_description[] = {
  {"default",
   "Default workstation model. Currently, CPU:Cas01 and network:LV08 (with cross traffic enabled)",
   surf_workstation_model_init_current_default},
  {"compound",
   "Workstation model that is automatically chosen if you change the network and CPU models",
   surf_workstation_model_init_compound},
  {"ptask_L07", "Workstation model somehow similar to Cas01+CM02 but allowing parallel tasks",
   surf_workstation_model_init_ptask_L07},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_optimization_mode_description[] = {
  {"Lazy",
   "Lazy action management (partial invalidation in lmm + heap in action remaining).",
   NULL},
  {"TI",
   "Trace integration. Highly optimized mode when using availability traces (only available for the Cas01 CPU model for now).",
    NULL},
  {"Full",
   "Full update of remaining and variables. Slow but may be useful when debugging.",
   NULL},
  {NULL, NULL, NULL}      /* this array must be NULL terminated */
};

s_surf_model_description_t surf_storage_model_description[] = {
  {"default",
   "Simplistic storage model.",
   surf_storage_model_init_default},
  {NULL, NULL,  NULL}      /* this array must be NULL terminated */
};

#ifdef CONTEXT_THREADS
static xbt_parmap_t surf_parmap = NULL; /* parallel map on models */
#endif

static int surf_nthreads = 1;    /* number of threads of the parmap (1 means no parallelism) */
static double *surf_mins = NULL; /* return value of share_resources for each model */
static int surf_min_index;       /* current index in surf_mins */
static double min;               /* duration determined by surf_solve */

static void surf_share_resources(surf_model_t model);
static void surf_update_actions_state(surf_model_t model);

/** Displays the long description of all registered models, and quit */
void model_help(const char *category, s_surf_model_description_t * table)
{
  int i;
  printf("Long description of the %s models accepted by this simulator:\n",
         category);
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
        xbt_realloc(name_list,
                    strlen(name_list) + strlen(table[i].name) + 3);
    strcat(name_list, ", ");
    strcat(name_list, table[i].name);
  }
  xbt_die("Model '%s' is invalid! Valid models are: %s.", name, name_list);
  return -1;
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
       action;
       action = xbt_swag_getNext(action, running_actions->offset)) {
    value = lmm_variable_getvalue(VARIABLE(action));
    if (value > 0) {
      if (action->remains > 0)
        value = action->remains / value;
      else
        value = 0.0;
      if (value < min) {
        min = value;
        XBT_DEBUG("Updating min (value) with %p: %f", action, min);
      }
    }
    if ((action->max_duration >= 0) && (action->max_duration < min)) {
      min = action->max_duration;
      XBT_DEBUG("Updating min (duration) with %p: %f", action, min);
    }
  }
  XBT_DEBUG("min value : %f", min);

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
  XBT_DEBUG("Create all Libs");
  host_lib = xbt_lib_new();
  link_lib = xbt_lib_new();
  as_router_lib = xbt_lib_new();
  storage_lib = xbt_lib_new();

  XBT_DEBUG("ADD ROUTING LEVEL");
  ROUTING_HOST_LEVEL = xbt_lib_add_level(host_lib,xbt_free);
  ROUTING_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,xbt_free);
  ROUTING_STORAGE_LEVEL = xbt_lib_add_level(storage_lib,xbt_free);
  ROUTING_STORAGE_TYPE_LEVEL = xbt_lib_add_level(storage_lib,routing_storage_type_free);
  ROUTING_STORAGE_HOST_LEVEL = xbt_lib_add_level(storage_lib,routing_storage_host_free);

  XBT_DEBUG("ADD SURF LEVELS");
  SURF_CPU_LEVEL = xbt_lib_add_level(host_lib,surf_resource_free);
  SURF_WKS_LEVEL = xbt_lib_add_level(host_lib,surf_resource_free);
  SURF_LINK_LEVEL = xbt_lib_add_level(link_lib,surf_resource_free);

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
  surf_action_init();
  if (MC_IS_ENABLED)
    MC_memory_init();
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
  routing_exit();

  if (maxmin_system) {
    lmm_system_free(maxmin_system);
    maxmin_system = NULL;
  }
  if (history) {
    tmgr_history_free(history);
    history = NULL;
  }
  surf_action_exit();

#ifdef CONTEXT_THREADS
  xbt_parmap_destroy(surf_parmap);
  xbt_free(surf_mins);
  surf_mins = NULL;
#endif

  xbt_dynar_free(&surf_path);

  xbt_lib_free(&host_lib);
  xbt_lib_free(&link_lib);
  xbt_lib_free(&as_router_lib);
  xbt_lib_free(&storage_lib);

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

  XBT_DEBUG
      ("First Run! Let's \"purge\" events and put models in the right state");
  while ((next_event_date = tmgr_history_next_date(history)) != -1.0) {
    if (next_event_date > NOW)
      break;
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value,
                                            (void **) &resource))) {
      resource->model->model_private->update_resource_state(resource,
                                                            event, value,
                                                            NOW);
    }
  }
  xbt_dynar_foreach(model_list, iter, model)
      model->model_private->update_actions_state(NOW, 0.0);
}

double surf_solve(double max_date)
{
  min = -1.0; /* duration */
  double next_event_date = -1.0;
  double model_next_action_end = -1.0;
  double value = -1.0;
  surf_resource_t resource = NULL;
  surf_model_t model = NULL;
  tmgr_trace_event_t event = NULL;
  unsigned int iter;

  if (max_date != -1.0 && max_date != NOW) {
    min = max_date - NOW;
  }

  XBT_DEBUG("Looking for next action end for all models except NS3");

  if (surf_mins == NULL) {
    surf_mins = xbt_new(double, xbt_dynar_length(model_list));
  }
  surf_min_index = 0;


  if (surf_get_nthreads() > 1) {
    /* parallel version */
#ifdef CONTEXT_THREADS
    xbt_parmap_apply(surf_parmap, (void_f_pvoid_t) surf_share_resources, model_list);
#endif
  }
  else {
    /* sequential version */
    xbt_dynar_foreach(model_list, iter, model) {
      surf_share_resources(model);
    }
  }

  unsigned i;
  for (i = 0; i < xbt_dynar_length(model_list); i++) {
    if ((min < 0.0 || surf_mins[i] < min)
        && surf_mins[i] >= 0.0) {
      min = surf_mins[i];
    }
  }

  XBT_DEBUG("Min for resources (remember that NS3 dont update that value) : %f", min);

  XBT_DEBUG("Looking for next trace event");

  do {
    XBT_DEBUG("Next TRACE event : %f", next_event_date);

    next_event_date = tmgr_history_next_date(history);

    if(surf_network_model->name && !strcmp(surf_network_model->name,"network NS3")){
      if(next_event_date!=-1.0 && min!=-1.0) {
        min = MIN(next_event_date - NOW, min);
      } else{
        min = MAX(next_event_date - NOW, min);
      }

      XBT_DEBUG("Run for NS3 at most %f", min);
      // run until min or next flow
      model_next_action_end = surf_network_model->model_private->share_resources(min);

      XBT_DEBUG("Min for NS3 : %f", model_next_action_end);
      if(model_next_action_end>=0.0)
        min = model_next_action_end;
    }

    if (next_event_date == -1.0) {
    	XBT_DEBUG("no next TRACE event. Stop searching for it");
    	break;
    }

    if ((min != -1.0) && (next_event_date > NOW + min)) break;

    XBT_DEBUG("Updating models");
    while ((event =
            tmgr_history_get_next_event_leq(history, next_event_date,
                                            &value,
                                            (void **) &resource))) {
      if (resource->model->model_private->resource_used(resource)) {
        min = next_event_date - NOW;
        XBT_DEBUG
            ("This event will modify model state. Next event set to %f",
             min);
      }
      /* update state of model_obj according to new value. Does not touch lmm.
         It will be modified if needed when updating actions */
      XBT_DEBUG("Calling update_resource_state for resource %s with min %lf",
             resource->model->name, min);
      resource->model->model_private->update_resource_state(resource,
                                                            event, value,
                                                            NOW + min);
    }
  } while (1);

  /* FIXME: Moved this test to here to avoid stopping simulation if there are actions running on cpus and all cpus are with availability = 0.
   * This may cause an infinite loop if one cpu has a trace with periodicity = 0 and the other a trace with periodicity > 0.
   * The options are: all traces with same periodicity(0 or >0) or we need to change the way how the events are managed */
  if (min == -1.0) {
	XBT_DEBUG("No next event at all. Bail out now.");
    return -1.0;
  }

  XBT_DEBUG("Duration set to %f", min);

  NOW = NOW + min;

  if (surf_get_nthreads() > 1) {
    /* parallel version */
#ifdef CONTEXT_THREADS
    xbt_parmap_apply(surf_parmap, (void_f_pvoid_t) surf_update_actions_state, model_list);
#endif
  }
  else {
    /* sequential version */
    xbt_dynar_foreach(model_list, iter, model) {
      surf_update_actions_state(model);
    }
  }

#ifdef HAVE_TRACING
  TRACE_paje_dump_buffer (0);
#endif

  return min;
}

XBT_INLINE double surf_get_clock(void)
{
  return NOW;
}

static void surf_share_resources(surf_model_t model)
{
  if (strcmp(model->name,"network NS3")) {
    XBT_DEBUG("Running for Resource [%s]", model->name);
    double next_action_end = model->model_private->share_resources(NOW);
    XBT_DEBUG("Resource [%s] : next action end = %f",
        model->name, next_action_end);
    int i = __sync_fetch_and_add(&surf_min_index, 1);
    surf_mins[i] = next_action_end;
  }
}

static void surf_update_actions_state(surf_model_t model)
{
  model->model_private->update_actions_state(NOW, min);
}

/**
 * \brief Returns the number of parallel threads used to update the models.
 * \return the number of threads (1 means no parallelism)
 */
int surf_get_nthreads(void) {
  return surf_nthreads;
}

/**
 * \brief Sets the number of parallel threads used to update the models.
 *
 * A value of 1 means no parallelism.
 *
 * \param nb_threads the number of threads to use
 */
void surf_set_nthreads(int nthreads) {

  if (nthreads<=0) {
     nthreads = xbt_os_get_numcores();
     XBT_INFO("Auto-setting surf/nthreads to %d",nthreads);
  }

#ifdef CONTEXT_THREADS
  xbt_parmap_destroy(surf_parmap);
  surf_parmap = NULL;
#endif

  if (nthreads > 1) {
#ifdef CONTEXT_THREADS
    surf_parmap = xbt_parmap_new(nthreads, XBT_PARMAP_DEFAULT);
#else
    THROWF(arg_error, 0, "Cannot activate parallel threads in Surf: your architecture does not support threads");
#endif
  }

  surf_nthreads = nthreads;
}
