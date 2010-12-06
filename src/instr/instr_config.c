/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */


#include "instr/instr_private.h"

#ifdef HAVE_TRACING

XBT_LOG_NEW_CATEGORY(instr, "Logging the behavior of the tracing system (used for Visualization/Analysis of simulations)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_config, instr, "Configuration");

#define OPT_TRACING               "tracing"
#define OPT_TRACING_SMPI          "tracing/smpi"
#define OPT_TRACING_SMPI_GROUP    "tracing/smpi/group"
#define OPT_TRACING_PLATFORM      "tracing/platform"
#define OPT_TRACING_UNCATEGORIZED "tracing/uncategorized"
#define OPT_TRACING_MSG_TASK      "tracing/msg/task"
#define OPT_TRACING_MSG_PROCESS   "tracing/msg/process"
#define OPT_TRACING_MSG_VOLUME    "tracing/msg/volume"
#define OPT_TRACING_FILENAME      "tracing/filename"
#define OPT_TRACING_PLATFORM_METHOD "tracing/platform/method"
#define OPT_TRIVA_UNCAT_CONF      "triva/uncategorized"
#define OPT_TRIVA_CAT_CONF        "triva/categorized"

static int trace_configured = 0;
static int trace_active = 0;

extern xbt_dict_t created_categories; //declared in instr_interface.c

void TRACE_activate (void)
{
  xbt_assert0 (trace_active==0, "Tracing is already active.");
  trace_active = 1;
  DEBUG0 ("Tracing is on");
}

void TRACE_desactivate (void)
{
  trace_active = 0;
  DEBUG0 ("Tracing is off");
}

int TRACE_is_active (void)
{
  return trace_active;
}

int TRACE_is_enabled(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING);
}

int TRACE_is_configured(void)
{
  return trace_configured;
}

int TRACE_smpi_is_enabled(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI);
}

int TRACE_smpi_is_grouped(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_SMPI_GROUP);
}

int TRACE_platform_is_enabled(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_PLATFORM);
}

int TRACE_uncategorized (void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_UNCATEGORIZED);
}

int TRACE_msg_task_is_enabled(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_TASK);
}

int TRACE_msg_process_is_enabled(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_PROCESS);
}

int TRACE_msg_volume_is_enabled(void)
{
  return xbt_cfg_get_int(_surf_cfg_set, OPT_TRACING_MSG_VOLUME);
}

char *TRACE_get_filename(void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRACING_FILENAME);
}

char *TRACE_get_platform_method(void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRACING_PLATFORM_METHOD);
}

char *TRACE_get_triva_uncat_conf (void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRIVA_UNCAT_CONF);
}

char *TRACE_get_triva_cat_conf (void)
{
  return xbt_cfg_get_string(_surf_cfg_set, OPT_TRIVA_CAT_CONF);
}

void TRACE_global_init(int *argc, char **argv)
{
  /* name of the tracefile */
  char *default_tracing_filename = xbt_strdup("simgrid.trace");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_FILENAME,
                   "Trace file created by the instrumented SimGrid.",
                   xbt_cfgelm_string, &default_tracing_filename, 1, 1,
                   NULL, NULL);

  /* tracing */
  int default_tracing = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING,
                   "Enable Tracing.",
                   xbt_cfgelm_int, &default_tracing, 0, 1,
                   NULL, NULL);

  /* smpi */
  int default_tracing_smpi = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_SMPI,
                   "Tracing of the SMPI interface.",
                   xbt_cfgelm_int, &default_tracing_smpi, 0, 1,
                   NULL, NULL);

  /* smpi grouped */
  int default_tracing_smpi_grouped = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_SMPI_GROUP,
                   "Group MPI processes by host.",
                   xbt_cfgelm_int, &default_tracing_smpi_grouped, 0, 1,
                   NULL, NULL);


  /* platform */
  int default_tracing_platform = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_PLATFORM,
                   "Tracing of categorized platform (host and link) utilization.",
                   xbt_cfgelm_int, &default_tracing_platform, 0, 1,
                   NULL, NULL);

  /* tracing uncategorized resource utilization */
  int default_tracing_uncategorized = 0;
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_UNCATEGORIZED,
                   "Tracing of uncategorized resource (host and link) utilization.",
                   xbt_cfgelm_int, &default_tracing_uncategorized, 0, 1,
                   NULL, NULL);

  /* platform method */
  char *default_tracing_platform_method = xbt_strdup("a");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRACING_PLATFORM_METHOD,
                   "Tracing method used to register categorized resource behavior.",
                   xbt_cfgelm_string, &default_tracing_platform_method, 1,
                   1, NULL, NULL);

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

  /* Triva graph configuration for uncategorized tracing */
  char *default_triva_uncat_conf_file = xbt_strdup ("");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRIVA_UNCAT_CONF,
                   "Triva Graph configuration file for uncategorized resource utilization traces.",
                   xbt_cfgelm_string, &default_triva_uncat_conf_file, 1, 1,
                   NULL, NULL);

  /* Triva graph configuration for uncategorized tracing */
  char *default_triva_cat_conf_file = xbt_strdup ("");
  xbt_cfg_register(&_surf_cfg_set, OPT_TRIVA_CAT_CONF,
                   "Triva Graph configuration file for categorized resource utilization traces.",
                   xbt_cfgelm_string, &default_triva_cat_conf_file, 1, 1,
                   NULL, NULL);

  /* instrumentation can be considered configured now */
  trace_configured = 1;
}

static void print_line (const char *option, const char *desc, const char *longdesc, int detailed)
{
  char str[INSTR_DEFAULT_STR_SIZE];
  snprintf (str, INSTR_DEFAULT_STR_SIZE, "--cfg=%s ", option);

  int len = strlen (str);
  printf ("%s%*.*s %s\n", str, 30-len, 30-len, "", desc);
  if (!!longdesc && detailed){
    printf ("%s\n\n", longdesc);
  }
}

void TRACE_help (int detailed)
{
  printf(
      "Description of the tracing options accepted by this simulator:\n\n");
  print_line (OPT_TRACING, "Enable the tracing system",
      "  It activates the tracing system and register the simulation platform\n"
      "  in the trace file. You have to enable this option to others take effect.",
      detailed);
  print_line (OPT_TRACING_PLATFORM, "Trace categorized resource utilization",
      "  It activates the categorized resource utilization tracing. It should\n"
      "  be enabled if tracing categories are used by this simulator.",
      detailed);
  print_line (OPT_TRACING_UNCATEGORIZED, "Trace uncategorized resource utilization",
      "  It activates the uncategorized resource utilization tracing. Use it if\n"
      "  this simulator do not use tracing categories and resource use have to be\n"
      "  traced.",
      detailed);
  print_line (OPT_TRACING_PLATFORM_METHOD, "Change the resource utilization tracing method",
      "  It changes the way resource utilization (categorized or not) is traced\n"
      "  inside the simulation core. Method 'a' (default) traces all updates defined\n"
      "  by the CPU/network model of a given resource. Depending on the interface used\n"
      "  by this simulator (MSG, SMPI, SimDAG), the default method can generate large\n"
      "  trace files. Method 'b' tries to make smaller tracefiles using clever updates,\n"
      "  without losing details of resource utilization. Method 'c' generates even\n"
      "  smaller files by doing time integration during the simulation, but it loses\n"
      "  precision. If this last method is used, the smallest timeslice used in the\n"
      "  tracefile analysis must be bigger than the smaller resource utilization. If\n"
      "  unsure, do not change this option.",
      detailed);
  print_line (OPT_TRACING_FILENAME, "Filename to register traces",
      "  A file with this name will be created to register the simulation. The file\n"
      "  is in the Paje format and can be analyzed using Triva or Paje visualization\n"
      "  tools. More information can be found in these webpages:\n"
      "     http://triva.gforge.inria.fr/\n"
      "     http://paje.sourceforge.net/",
      detailed);
  print_line (OPT_TRACING_SMPI, "Trace the MPI Interface (SMPI)",
      "  This option only has effect if this simulator is SMPI-based. Traces the MPI\n"
      "  interface and generates a trace that can be analyzed using Gantt-like\n"
      "  visualizations. Every MPI function (implemented by SMPI) is transformed in a\n"
      "  state, and point-to-point communications can be analyzed with arrows.",
      detailed);
  print_line (OPT_TRACING_SMPI_GROUP, "Group MPI processes by host (SMPI)",
      "  This option only has effect if this simulator is SMPI-based. The processes\n"
      "  are grouped by the hosts where they were executed.",
      detailed);
  print_line (OPT_TRACING_MSG_TASK, "Trace task behavior (MSG)",
      "  This option only has effect if this simulator is MSG-based. It traces the\n"
      "  behavior of all categorized MSG tasks, grouping them by hosts.",
      detailed);
  print_line (OPT_TRACING_MSG_PROCESS, "Trace processes behavior (MSG)",
      "  This option only has effect if this simulator is MSG-based. It traces the\n"
      "  behavior of all categorized MSG processes, grouping them by hosts. This option\n"
      "  can be used to track process location if this simulator has process migration.",
      detailed);
  print_line (OPT_TRACING_MSG_VOLUME, "Tracing of communication volume (MSG)",
      "  This experimental option only has effect if this simulator is MSG-based.\n"
      "  It traces the communication volume of MSG send/receive.",
      detailed);
  print_line (OPT_TRIVA_UNCAT_CONF, "Generate graph configuration for Triva",
      "  This option can be used in all types of simulators build with SimGrid\n"
      "  to generate a uncategorized resource utilization graph to be used as\n"
      "  configuration for the Triva visualization analysis. This option\n"
      "  can be used with tracing/categorized:1 and tracing:1 options to\n"
      "  analyze an unmodified simulator before changing it to contain\n"
      "  categories.",
      detailed);
  print_line (OPT_TRIVA_CAT_CONF, "generate uncategorized graph configuration for Triva",
      "  This option can be used if this simulator uses tracing categories\n"
      "  in its code. The file specified by this option holds a graph configuration\n"
      "  file for the Triva visualization tool that can be used to analyze a categorized\n"
      "  resource utilization.",
      detailed);
}

void TRACE_generate_triva_uncat_conf (void)
{
  char *output = TRACE_get_triva_uncat_conf ();
  if (output && strlen(output) > 0){
    FILE *file = fopen (output, "w");
    xbt_assert1 (file != NULL,
       "Unable to open file (%s) for writing triva graph "
       "configuration (uncategorized).", output);
    fprintf (file,
        "{\n"
        "  node = (HOST);\n"
        "  edge = (LINK);\n"
        "\n"
        "  HOST = {\n"
        "    size = power;\n"
        "    scale = global;\n"
        "    host_sep = {\n"
        "      type = separation;\n"
        "      size = power;\n"
        "      values = (power_used);\n"
        "    };\n"
        "  };\n"
        "  LINK = {\n"
        "    src = source;\n"
        "    dst = destination;\n"
        "    size = bandwidth;\n"
        "    scale = global;\n"
        "    link_sep = {\n"
        "      type = separation;\n"
        "      size = bandwidth;\n"
        "      values = (bandwidth_used);\n"
        "    };\n"
        "  };\n"
        "  graphviz-algorithm = neato;\n"
        "}\n"
        );
    fclose (file);
  }
}

void TRACE_generate_triva_cat_conf (void)
{
  char *output = TRACE_get_triva_cat_conf();
  if (output && strlen(output) > 0){
    //check if we do have categories declared
    if (xbt_dict_length(created_categories) == 0){
      INFO0("No categories declared, ignoring generation of triva graph configuration");
      return;
    }
    xbt_dict_cursor_t cursor=NULL;
    char *key, *data;
    FILE *file = fopen (output, "w");
    xbt_assert1 (file != NULL,
       "Unable to open file (%s) for writing triva graph "
       "configuration (categorized).", output);
    fprintf (file,
        "{\n"
        "  node = (HOST);\n"
        "  edge = (LINK);\n"
        "\n"
        "  HOST = {\n"
        "    size = power;\n"
        "    scale = global;\n"
        "    host_sep = {\n"
        "      type = separation;\n"
        "      size = power;\n"
        "      values = (");
    xbt_dict_foreach(created_categories,cursor,key,data) {
      fprintf(file, "p%s, ",key);
    }
    fprintf (file,
        ");\n"
        "    };\n"
        "  };\n"
        "  LINK = {\n"
        "    src = source;\n"
        "    dst = destination;\n"
        "    size = bandwidth;\n"
        "    scale = global;\n"
        "    link_sep = {\n"
        "      type = separation;\n"
        "      size = bandwidth;\n"
        "      values = (");
    xbt_dict_foreach(created_categories,cursor,key,data) {
      fprintf(file, "b%s, ",key);
    }
    fprintf (file,
        ");\n"
        "    };\n"
        "  };\n"
        "  graphviz-algorithm = neato;\n"
        "}\n"
        );
    fclose(file);
  }
}

#undef OPT_TRACING
#undef OPT_TRACING_SMPI
#undef OPT_TRACING_SMPI_GROUP
#undef OPT_TRACING_PLATFORM
#undef OPT_TRACING_UNCATEGORIZED
#undef OPT_TRACING_MSG_TASK
#undef OPT_TRACING_MSG_PROCESS
#undef OPT_TRACING_MSG_VOLUME
#undef OPT_TRACING_FILENAME
#undef OPT_TRACING_PLATFORM_METHOD
#undef OPT_TRIVA_UNCAT_CONF
#undef OPT_TRIVA_CAT_CONF

#endif /* HAVE_TRACING */
