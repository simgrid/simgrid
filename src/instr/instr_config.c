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

static int trace_configured = 0;

int _TRACE_configured (void)
{
  return trace_configured;
}

int _TRACE_smpi_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI);
}

int _TRACE_platform_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_PLATFORM);
}

int _TRACE_msg_task_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_TASK);
}

int _TRACE_msg_process_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_PROCESS);
}

int _TRACE_msg_volume_enabled (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_VOLUME);
}

char *_TRACE_filename (void)
{
  return xbt_cfg_get_string (_surf_cfg_set, OPT_TRACING_FILENAME);
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
