/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */


#include "instr/private.h"

#ifdef HAVE_TRACING

#define OPT_TRACING_SMPI          "tracing/smpi"
#define OPT_TRACING_PLATFORM      "tracing/platform"
#define OPT_TRACING_MSG_TASK      "tracing/msg/task"
#define OPT_TRACING_MSG_PROCESS   "tracing/msg/process"
#define OPT_TRACING_MSG_VOLUME    "tracing/msg/volume"
#define OPT_TRACING_FILENAME      "tracing/filename"
#define OPT_TRACING_PLATFORM_METHOD "tracing/platform/method"

static int trace_configured = 0;

int TRACE_is_configured (void)
{
  return trace_configured;
}

int TRACE_smpi_is_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI);
}

int TRACE_platform_is_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_PLATFORM);
}

int TRACE_msg_task_is_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_TASK);
}

int TRACE_msg_process_is_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_PROCESS);
}

int TRACE_msg_volume_is_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_VOLUME);
}

char *TRACE_get_filename (void)
{
  return xbt_cfg_get_string (_surf_cfg_set, OPT_TRACING_FILENAME);
}

char *TRACE_get_platform_method (void)
{
  return xbt_cfg_get_string (_surf_cfg_set, OPT_TRACING_PLATFORM_METHOD);
}

void TRACE_global_init(int *argc, char **argv)
{
  /* name of the tracefile */
  char *default_tracing_filename = xbt_strdup("simgrid.trace");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_FILENAME,
                    "Trace file created by the instrumented SimGrid.",
                    xbt_cfgelm_string, &default_tracing_filename, 1, 1,
                    NULL, NULL);

  /* smpi */
  int default_tracing_smpi = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_SMPI,
                   "Tracing of the SMPI interface.",
                   xbt_cfgelm_int, &default_tracing_smpi, 0, 1,
                   NULL, NULL);

  /* platform */
  int default_tracing_platform = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_PLATFORM,
                   "Tracing of categorized platform (host and link) utilization.",
                   xbt_cfgelm_int, &default_tracing_platform, 0, 1,
                   NULL, NULL);

  /* platform method */
  char *default_tracing_platform_method = xbt_strdup ("b");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_PLATFORM_METHOD,
                   "Tracing method used to register categorized resource behavior.",
                   xbt_cfgelm_string, &default_tracing_platform_method, 1, 1,
                   NULL, NULL);

  /* msg task */
  int default_tracing_msg_task = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_MSG_TASK,
                   "Tracing of MSG task behavior.",
                   xbt_cfgelm_int, &default_tracing_msg_task, 0, 1,
                   NULL, NULL);

  /* msg process */
  int default_tracing_msg_process = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_MSG_PROCESS,
                   "Tracing of MSG process behavior.",
                   xbt_cfgelm_int, &default_tracing_msg_process, 0, 1,
                   NULL, NULL);

  /* msg volume (experimental) */
  int default_tracing_msg_volume = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_MSG_VOLUME,
                   "Tracing of MSG communication volume (experimental).",
                   xbt_cfgelm_int, &default_tracing_msg_volume, 0, 1,
                   NULL, NULL);

  /* instrumentation can be considered configured now */
  trace_configured = 1;
}

#endif
