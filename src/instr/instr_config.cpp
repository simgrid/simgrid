/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "simgrid/sg_config.h"
#include "surf/surf.h"

XBT_LOG_NEW_CATEGORY(instr, "Logging the behavior of the tracing system (used for Visualization/Analysis of simulations)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_config, instr, "Configuration");

#define OPT_TRACING_BASIC                "tracing/basic"
#define OPT_TRACING_BUFFER               "tracing/buffer"
#define OPT_TRACING_CATEGORIZED          "tracing/categorized"
#define OPT_TRACING_COMMENT_FILE         "tracing/comment_file"
#define OPT_TRACING_COMMENT              "tracing/comment"
#define OPT_TRACING_DISABLE_DESTROY      "tracing/disable_destroy"
#define OPT_TRACING_DISABLE_LINK         "tracing/disable_link"
#define OPT_TRACING_DISABLE_POWER        "tracing/disable_power"
#define OPT_TRACING_DISPLAY_SIZES        "tracing/smpi/display_sizes"
#define OPT_TRACING_FILENAME             "tracing/filename"
#define OPT_TRACING_FORMAT_TI_ONEFILE    "tracing/smpi/format/ti_one_file"
#define OPT_TRACING_FORMAT               "tracing/smpi/format"
#define OPT_TRACING_MSG_PROCESS          "tracing/msg/process"
#define OPT_TRACING_MSG_VM               "tracing/msg/vm"
#define OPT_TRACING_ONELINK_ONLY         "tracing/onelink_only"
#define OPT_TRACING_PLATFORM             "tracing/platform"
#define OPT_TRACING_PRECISION            "tracing/precision"
#define OPT_TRACING_SMPI_COMPUTING       "tracing/smpi/computing"
#define OPT_TRACING_SMPI_GROUP           "tracing/smpi/group"
#define OPT_TRACING_SMPI_INTERNALS       "tracing/smpi/internals"
#define OPT_TRACING_SMPI_SLEEPING        "tracing/smpi/sleeping"
#define OPT_TRACING_SMPI                 "tracing/smpi"
#define OPT_TRACING_TOPOLOGY             "tracing/platform/topology"
#define OPT_TRACING                      "tracing"
#define OPT_TRACING_UNCATEGORIZED        "tracing/uncategorized"
#define OPT_VIVA_CAT_CONF                "viva/categorized"
#define OPT_VIVA_UNCAT_CONF              "viva/uncategorized"

static int trace_enabled = 0;
static int trace_platform;
static int trace_platform_topology;
static int trace_smpi_enabled;
static int trace_smpi_grouped;
static int trace_smpi_computing;
static int trace_smpi_sleeping;
static int trace_view_internals;
static int trace_categorized;
static int trace_uncategorized;
static int trace_msg_process_enabled;
static int trace_msg_vm_enabled;
static int trace_buffer;
static int trace_onelink_only;
static int trace_disable_destroy;
static int trace_basic;
static int trace_display_sizes = 0;
static int trace_disable_link;
static int trace_disable_power;
static int trace_precision;

static int trace_configured = 0;
static int trace_active = 0;

static void TRACE_getopts(void)
{
  trace_enabled = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING);
  trace_platform = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_PLATFORM);
  trace_platform_topology = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_TOPOLOGY);
  trace_smpi_enabled = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_SMPI);
  trace_smpi_grouped = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_SMPI_GROUP);
  trace_smpi_computing = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_SMPI_COMPUTING);
  trace_smpi_sleeping = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_SMPI_SLEEPING);
  trace_view_internals = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_SMPI_INTERNALS);
  trace_categorized = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_CATEGORIZED);
  trace_uncategorized = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_UNCATEGORIZED);
  trace_msg_process_enabled = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_MSG_PROCESS);
  trace_msg_vm_enabled = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_MSG_VM);
  trace_buffer = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_BUFFER);
  trace_onelink_only = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_ONELINK_ONLY);
  trace_disable_destroy = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_DISABLE_DESTROY);
  trace_basic = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_BASIC);
  trace_display_sizes = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_DISPLAY_SIZES);
  trace_disable_link = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_DISABLE_LINK);
  trace_disable_power = xbt_cfg_get_boolean(_sg_cfg_set, OPT_TRACING_DISABLE_POWER);
  trace_precision = xbt_cfg_get_int(_sg_cfg_set, OPT_TRACING_PRECISION);
}

static xbt_dynar_t TRACE_start_functions = NULL;
void TRACE_add_start_function(void (*func) ())
{
  if (TRACE_start_functions == NULL)
    TRACE_start_functions = xbt_dynar_new(sizeof(void (*)()), NULL);
  xbt_dynar_push(TRACE_start_functions, &func);
}

int TRACE_start()
{
  if (TRACE_is_configured())
    TRACE_getopts();

  // tracing system must be:
  //    - enabled (with --cfg=tracing:yes)
  //    - already configured (TRACE_global_init already called)
  if (TRACE_is_enabled()) {

    XBT_DEBUG("Tracing starts");
    /* init the tracing module to generate the right output */
    /* open internal buffer */
    TRACE_init();

    /* open the trace file(s) */
    const char* format = sg_cfg_get_string(OPT_TRACING_FORMAT);
    XBT_DEBUG("Tracing format %s\n", format);
    if(!strcmp(format, "Paje")){
      TRACE_paje_init();
      TRACE_paje_start();
    }else if (!strcmp(format, "TI")){
      TRACE_TI_init();
      TRACE_TI_start();
    }else{
      xbt_die("Unknown trace format :%s ", format);
    }

    /* activate trace */
    if (trace_active == 1) {
      THROWF(tracing_error, 0, "Tracing is already active");
    }
    trace_active = 1;
    XBT_DEBUG("Tracing is on");

    /* other trace initialization */
    created_categories = xbt_dict_new_homogeneous(xbt_free_f);
    declared_marks = xbt_dict_new_homogeneous(xbt_free_f);
    user_host_variables = xbt_dict_new_homogeneous(xbt_free_f);
    user_vm_variables = xbt_dict_new_homogeneous(xbt_free_f);
    user_link_variables = xbt_dict_new_homogeneous(xbt_free_f);

    if (TRACE_start_functions != NULL) {
      void (*func) ();
      unsigned int iter;
      xbt_dynar_foreach(TRACE_start_functions, iter, func) {
        func();
      }
    }
  }
  xbt_dynar_free(&TRACE_start_functions);
  return 0;
}

static xbt_dynar_t TRACE_end_functions = NULL;
void TRACE_add_end_function(void (*func) (void))
{
  if (TRACE_end_functions == NULL)
    TRACE_end_functions = xbt_dynar_new(sizeof(void (*)(void)), NULL);
  xbt_dynar_push(TRACE_end_functions, &func);
}

int TRACE_end()
{
  int retval;
  if (!trace_active) {
    retval = 1;
  } else {
    retval = 0;

    TRACE_generate_viva_uncat_conf();
    TRACE_generate_viva_cat_conf();

    /* dump trace buffer */
    TRACE_last_timestamp_to_dump = surf_get_clock();
    TRACE_paje_dump_buffer(1);

    /* destroy all data structures of tracing (and free) */
    PJ_container_free_all();
    PJ_type_free_all();
    PJ_container_release();
    PJ_type_release();

    if (TRACE_end_functions != NULL) {
      void (*func) (void);
      unsigned int iter;
      xbt_dynar_foreach(TRACE_end_functions, iter, func) {
        func();
      }
    }

    xbt_dict_free(&user_link_variables);
    xbt_dict_free(&user_host_variables);
    xbt_dict_free(&user_vm_variables);
    xbt_dict_free(&declared_marks);
    xbt_dict_free(&created_categories);

    /* close the trace files */
    const char* format = sg_cfg_get_string(OPT_TRACING_FORMAT);
    XBT_DEBUG("Tracing format %s\n", format);
    if(!strcmp(format, "Paje")){
      TRACE_paje_end();
    }else if (!strcmp(format, "TI")){
      TRACE_TI_end();
    }else{
      xbt_die("Unknown trace format :%s ", format);
    }
    /* close internal buffer */
    TRACE_finalize();
    /* de-activate trace */
    trace_active = 0;
    XBT_DEBUG("Tracing is off");
    XBT_DEBUG("Tracing system is shutdown");
  }
  xbt_dynar_free(&TRACE_start_functions); /* useful when exiting early */
  xbt_dynar_free(&TRACE_end_functions);
  return retval;
}

int TRACE_needs_platform (void)
{
  return TRACE_msg_process_is_enabled() || TRACE_msg_vm_is_enabled() || TRACE_categorized() ||
         TRACE_uncategorized() || TRACE_platform () || (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped());
}

int TRACE_is_enabled(void)
{
  return trace_enabled;
}

int TRACE_platform(void)
{
  return trace_platform;
}

int TRACE_platform_topology(void)
{
  return trace_platform_topology;
}

int TRACE_is_configured(void)
{
  return trace_configured;
}

int TRACE_smpi_is_enabled(void)
{
  return (trace_smpi_enabled || TRACE_smpi_is_grouped()) && TRACE_is_enabled();
}

int TRACE_smpi_is_grouped(void)
{
  return trace_smpi_grouped;
}

int TRACE_smpi_is_computing(void)
{
  return trace_smpi_computing;
}

int TRACE_smpi_is_sleeping(void)
{
  return trace_smpi_sleeping;
}

int TRACE_smpi_view_internals(void)
{
  return trace_view_internals;
}

int TRACE_categorized (void)
{
  return trace_categorized;
}

int TRACE_uncategorized (void)
{
  return trace_uncategorized;
}

int TRACE_msg_process_is_enabled(void)
{
  return trace_msg_process_enabled && TRACE_is_enabled();
}

int TRACE_msg_vm_is_enabled(void)
{
  return trace_msg_vm_enabled && TRACE_is_enabled();
}

int TRACE_disable_link(void)
{
  return trace_disable_link && TRACE_is_enabled();
}

int TRACE_disable_speed(void)
{
  return trace_disable_power && TRACE_is_enabled();
}

int TRACE_buffer (void)
{
  return trace_buffer && TRACE_is_enabled();
}

int TRACE_onelink_only (void)
{
  return trace_onelink_only && TRACE_is_enabled();
}

int TRACE_disable_destroy (void)
{
  return trace_disable_destroy && TRACE_is_enabled();
}

int TRACE_basic (void)
{
  return trace_basic && TRACE_is_enabled();
}

int TRACE_display_sizes (void)
{
   return trace_display_sizes && trace_smpi_enabled && TRACE_is_enabled();
}

char *TRACE_get_comment (void)
{
  return xbt_cfg_get_string(_sg_cfg_set, OPT_TRACING_COMMENT);
}

char *TRACE_get_comment_file (void)
{
  return xbt_cfg_get_string(_sg_cfg_set, OPT_TRACING_COMMENT_FILE);
}

int TRACE_precision (void)
{
  return xbt_cfg_get_int(_sg_cfg_set, OPT_TRACING_PRECISION);
}

char *TRACE_get_filename(void)
{
  return xbt_cfg_get_string(_sg_cfg_set, OPT_TRACING_FILENAME);
}

char *TRACE_get_viva_uncat_conf (void)
{
  return xbt_cfg_get_string(_sg_cfg_set, OPT_VIVA_UNCAT_CONF);
}

char *TRACE_get_viva_cat_conf (void)
{
  return xbt_cfg_get_string(_sg_cfg_set, OPT_VIVA_CAT_CONF);
}

void TRACE_global_init(int *argc, char **argv)
{
  static int is_initialised = 0;
  if (is_initialised) return;

  is_initialised = 1;
  /* name of the tracefile */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_FILENAME, "Trace file created by the instrumented SimGrid.",
                   xbt_cfgelm_string, 1, 1, NULL);
  xbt_cfg_setdefault_string(_sg_cfg_set, OPT_TRACING_FILENAME, "simgrid.trace");

  /* tracing */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING, "Enable Tracing.", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING, "no");

  /* register platform in the trace */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_PLATFORM, "Register the platform in the trace as a hierarchy.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_PLATFORM, "no");

  /* register platform in the trace */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_TOPOLOGY, "Register the platform topology in the trace as a graph.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_TOPOLOGY, "yes");

  /* smpi */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_SMPI, "Tracing of the SMPI interface.", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_SMPI, "no");

  /* smpi grouped */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_SMPI_GROUP, "Group MPI processes by host.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_SMPI_GROUP, "no");

  /* smpi computing */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_SMPI_COMPUTING,
                   "Generate states for timing out of SMPI parts of the application", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_SMPI_COMPUTING, "no");

/* smpi sleeping */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_SMPI_SLEEPING,
                   "Generate states for timing out of SMPI parts of the application", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_SMPI_SLEEPING, "no");

  /* smpi internals */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_SMPI_INTERNALS,
                   "View internal messages sent by Collective communications in SMPI", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_SMPI_INTERNALS, "no");

  /* tracing categorized resource utilization traces */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_CATEGORIZED,
                   "Tracing categorized resource utilization of hosts and links.", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_CATEGORIZED, "no");

  /* tracing uncategorized resource utilization */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_UNCATEGORIZED,
                   "Tracing uncategorized resource utilization of hosts and links.", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_UNCATEGORIZED, "no");

  /* msg process */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_MSG_PROCESS, "Tracing of MSG process behavior.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_MSG_PROCESS, "no");

  /* msg process */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_MSG_VM, "Tracing of MSG process behavior.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_MSG_VM, "no");

  /* disable tracing link */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_DISABLE_LINK, "Do not trace link bandwidth and latency.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_DISABLE_LINK, "no");

  /* disable tracing link */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_DISABLE_POWER, "Do not trace host power.", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_DISABLE_POWER, "no");

  /* tracing buffer */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_BUFFER, "Buffer trace events to put them in temporal order.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_BUFFER, "yes");

  /* tracing one link only */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_ONELINK_ONLY, "Use only routes with one link to trace platform.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_ONELINK_ONLY, "no");

  /* disable destroy */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_DISABLE_DESTROY, "Disable platform containers destruction.",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_DISABLE_DESTROY, "no");

  /* basic -- Avoid extended events (impoverished trace file) */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_BASIC, "Avoid extended events (impoverished trace file).",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_BASIC, "no");

  /* display_sizes -- Extended events with message size information */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_DISPLAY_SIZES,
                   "(smpi only for now) Extended events with message size information", xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_DISPLAY_SIZES, "no");

  /* format -- Switch the ouput format of Tracing */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_FORMAT, "(smpi only for now) Switch the output format of Tracing",
                   xbt_cfgelm_string, 1, 1, NULL);
  xbt_cfg_setdefault_string(_sg_cfg_set, OPT_TRACING_FORMAT, "Paje");

  /* format -- Switch the ouput format of Tracing */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_FORMAT_TI_ONEFILE,
                   "(smpi only for now) For replay format only : output to one file only",
                   xbt_cfgelm_boolean, 1, 1, NULL);
  xbt_cfg_setdefault_boolean(_sg_cfg_set, OPT_TRACING_FORMAT_TI_ONEFILE, "no");

  /* comment */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_COMMENT, "Comment to be added on the top of the trace file.",
                   xbt_cfgelm_string, 1, 1, NULL);
  xbt_cfg_setdefault_string(_sg_cfg_set, OPT_TRACING_COMMENT, "");

  /* comment_file */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_COMMENT_FILE,
                   "The contents of the file are added to the top of the trace file as comment.",
                   xbt_cfgelm_string, 1, 1, NULL);
  xbt_cfg_setdefault_string(_sg_cfg_set, OPT_TRACING_COMMENT_FILE, "");

  /* trace timestamp precision */
  xbt_cfg_register(&_sg_cfg_set, OPT_TRACING_PRECISION, "Numerical precision used when timestamping events (hence "
                   "this value is expressed in number of digits after decimal point)", xbt_cfgelm_int, 1, 1, NULL);
  xbt_cfg_setdefault_int(_sg_cfg_set, OPT_TRACING_PRECISION, 6);

  /* Viva graph configuration for uncategorized tracing */
  xbt_cfg_register(&_sg_cfg_set, OPT_VIVA_UNCAT_CONF,
                   "Viva Graph configuration file for uncategorized resource utilization traces.",
                   xbt_cfgelm_string, 1, 1, NULL);
  xbt_cfg_setdefault_string(_sg_cfg_set, OPT_VIVA_UNCAT_CONF, "");

  /* Viva graph configuration for uncategorized tracing */
  xbt_cfg_register(&_sg_cfg_set, OPT_VIVA_CAT_CONF,
                   "Viva Graph configuration file for categorized resource utilization traces.",
                   xbt_cfgelm_string, 1, 1, NULL);
  xbt_cfg_setdefault_string(_sg_cfg_set, OPT_VIVA_CAT_CONF, "");

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
  printf("Description of the tracing options accepted by this simulator:\n\n");
  print_line (OPT_TRACING, "Enable the tracing system",
      "  It activates the tracing system and register the simulation platform\n"
      "  in the trace file. You have to enable this option to others take effect.", detailed);
  print_line (OPT_TRACING_CATEGORIZED, "Trace categorized resource utilization",
      "  It activates the categorized resource utilization tracing. It should\n"
      "  be enabled if tracing categories are used by this simulator.", detailed);
  print_line (OPT_TRACING_UNCATEGORIZED, "Trace uncategorized resource utilization",
      "  It activates the uncategorized resource utilization tracing. Use it if\n"
      "  this simulator do not use tracing categories and resource use have to be\n"
      "  traced.", detailed);
  print_line (OPT_TRACING_FILENAME, "Filename to register traces",
      "  A file with this name will be created to register the simulation. The file\n"
      "  is in the Paje format and can be analyzed using Viva, Paje, and PajeNG visualization\n"
      "  tools. More information can be found in these webpages:\n"
      "     http://github.com/schnorr/viva/\n"
      "     http://github.com/schnorr/pajeng/\n"
      "     http://paje.sourceforge.net/", detailed);
  print_line (OPT_TRACING_SMPI, "Trace the MPI Interface (SMPI)",
      "  This option only has effect if this simulator is SMPI-based. Traces the MPI\n"
      "  interface and generates a trace that can be analyzed using Gantt-like\n"
      "  visualizations. Every MPI function (implemented by SMPI) is transformed in a\n"
      "  state, and point-to-point communications can be analyzed with arrows.", detailed);
  print_line (OPT_TRACING_SMPI_GROUP, "Group MPI processes by host (SMPI)",
      "  This option only has effect if this simulator is SMPI-based. The processes\n"
      "  are grouped by the hosts where they were executed.", detailed);
  print_line (OPT_TRACING_SMPI_COMPUTING, "Generates a \" Computing \" State",
      "  This option aims at tracing computations in the application, outside SMPI\n"
      "  to allow further study of simulated or real computation time", detailed);
   print_line (OPT_TRACING_SMPI_SLEEPING, "Generates a \" Sleeping \" State",
      "  This option aims at tracing sleeps in the application, outside SMPI\n"
      "  to allow further study of simulated or real sleep time", detailed);
  print_line (OPT_TRACING_SMPI_INTERNALS, "Generates tracing events corresponding",
      "  to point-to-point messages sent by collective communications", detailed);
  print_line (OPT_TRACING_MSG_PROCESS, "Trace processes behavior (MSG)",
      "  This option only has effect if this simulator is MSG-based. It traces the\n"
      "  behavior of all categorized MSG processes, grouping them by hosts. This option\n"
      "  can be used to track process location if this simulator has process migration.", detailed);
  print_line (OPT_TRACING_BUFFER, "Buffer events to put them in temporal order",
      "  This option put some events in a time-ordered buffer using the insertion\n"
      "  sort algorithm. The process of acquiring and releasing locks to access this\n"
      "  buffer and the cost of the sorting algorithm make this process slow. The\n"
      "  simulator performance can be severely impacted if this option is activated,\n"
      "  but you are sure to get a trace file with events sorted.", detailed);
  print_line (OPT_TRACING_ONELINK_ONLY, "Consider only one link routes to trace platform",
      "  This option changes the way SimGrid register its platform on the trace file.\n"
      "  Normally, the tracing considers all routes (no matter their size) on the\n"
      "  platform file to re-create the resource topology. If this option is activated,\n"
      "  only the routes with one link are used to register the topology within an AS.\n"
      "  Routes among AS continue to be traced as usual.", detailed);
  print_line (OPT_TRACING_DISABLE_DESTROY, "Disable platform containers destruction",
      "  Disable the destruction of containers at the end of simulation. This can be\n"
      "  used with simulators that have a different notion of time (different from\n"
      "  the simulated time).", detailed);
  print_line (OPT_TRACING_BASIC, "Avoid extended events (impoverished trace file).",
      "  Some visualization tools are not able to parse correctly the Paje file format.\n"
      "  Use this option if you are using one of these tools to visualize the simulation\n"
      "  trace. Keep in mind that the trace might be incomplete, without all the\n"
      "  information that would be registered otherwise.", detailed);
  print_line (OPT_TRACING_DISPLAY_SIZES, "Only works for SMPI now. Add message size information",
      "  Message size (in bytes) is added to links, and to states. For collectives,\n"
      "  the displayed value is the more relevant to the collective (total sent by\n"
      "  the process, usually)", detailed);
  print_line (OPT_TRACING_FORMAT, "Only works for SMPI now. Switch output format",
      "  Default format is Paje. Time independent traces are also supported,\n"
      "  to output traces that can later be used by the trace replay tool", detailed);
  print_line (OPT_TRACING_FORMAT_TI_ONEFILE, "Only works for SMPI now, and TI output format",
      "  By default, each process outputs to a separate file, inside a filename_files folder\n"
      "  By setting this option to yes, all processes will output to only one file\n"
      "  This is meant to avoid opening thousands of files with large simulations", detailed);
  print_line (OPT_TRACING_COMMENT, "Comment to be added on the top of the trace file.",
      "  Use this to add a comment line to the top of the trace file.", detailed);
  print_line (OPT_TRACING_COMMENT_FILE, "File contents added to trace file as comment.",
      "  Use this to add the contents of a file to the top of the trace file as comment.", detailed);
  print_line (OPT_VIVA_UNCAT_CONF, "Generate a graph configuration for Viva",
      "  This option can be used in all types of simulators build with SimGrid\n"
      "  to generate a uncategorized resource utilization graph to be used as\n"
      "  configuration for the Viva visualization tool. This option\n"
      "  can be used with tracing/categorized:1 and tracing:1 options to\n"
      "  analyze an unmodified simulator before changing it to contain\n"
      "  categories.", detailed);
  print_line (OPT_VIVA_CAT_CONF, "Generate an uncategorized graph configuration for Viva",
      "  This option can be used if this simulator uses tracing categories\n"
      "  in its code. The file specified by this option holds a graph configuration\n"
      "  file for the Viva visualization tool that can be used to analyze a categorized\n"
      "  resource utilization.", detailed);
  print_line (OPT_TRACING_TOPOLOGY, "Register the platform topology as a graph",
        "  This option (enabled by default) can be used to disable the tracing of\n"
        "  the platform topology in the trace file. Sometimes, such task is really\n"
        "  time consuming, since it must get the route from each host ot other hosts\n"
        "  within the same Autonomous System (AS).", detailed);
}

static void output_types (const char *name, xbt_dynar_t types, FILE *file)
{
  unsigned int i;
  fprintf (file, "  %s = (", name);
  for (i = xbt_dynar_length(types); i > 0; i--) {
    char *type = *(char**)xbt_dynar_get_ptr(types, i - 1);
    fprintf (file, "\"%s\"", type);
    if (i - 1 > 0){
      fprintf (file, ",");
    }else{
      fprintf (file, ");\n");
    }
  }
  xbt_dynar_free (&types);
}

static void output_categories (const char *name, xbt_dynar_t cats, FILE *file)
{
  unsigned int i;
  fprintf (file, "    values = (");
  for (i = xbt_dynar_length(cats); i > 0; i--) {
    char *cat = *(char**)xbt_dynar_get_ptr(cats, i - 1);
    fprintf (file, "\"%s%s\"", name, cat);
    if (i - 1 > 0){
      fprintf (file, ",");
    }else{
      fprintf (file, ");\n");
    }
  }
  xbt_dynar_free (&cats);
}

static void uncat_configuration (FILE *file)
{
  //register NODE and EDGE types
  output_types ("node", TRACE_get_node_types(), file);
  output_types ("edge", TRACE_get_edge_types(), file);
  fprintf (file, "\n");

  //configuration for all nodes
  fprintf (file,
      "  host = {\n"
      "    type = \"square\";\n"
      "    size = \"power\";\n"
      "    values = (\"power_used\");\n"
      "  };\n"
      "  link = {\n"
      "    type = \"rhombus\";\n"
      "    size = \"bandwidth\";\n"
      "    values = (\"bandwidth_used\");\n"
      "  };\n");
  //close
}

static void cat_configuration (FILE *file)
{
  //register NODE and EDGE types
  output_types ("node", TRACE_get_node_types(), file);
  output_types ("edge", TRACE_get_edge_types(), file);
  fprintf (file, "\n");

  //configuration for all nodes
  fprintf (file,
           "  host = {\n"
           "    type = \"square\";\n"
           "    size = \"power\";\n");
  output_categories ("p", TRACE_get_categories(), file);
  fprintf (file,
           "  };\n"
           "  link = {\n"
           "    type = \"rhombus\";\n"
           "    size = \"bandwidth\";\n");
  output_categories ("b", TRACE_get_categories(), file);
  fprintf (file, "  };\n");
  //close
}

static void generate_uncat_configuration (const char *output, const char *name, int brackets)
{
  if (output && strlen(output) > 0){
    FILE *file = fopen (output, "w");
    if (file == NULL){
      THROWF (system_error, 1, "Unable to open file (%s) for writing %s graph "
          "configuration (uncategorized).", output, name);
    }

    if (brackets) fprintf (file, "{\n");
    uncat_configuration (file);
    if (brackets) fprintf (file, "}\n");
    fclose (file);
  }
}

static void generate_cat_configuration (const char *output, const char *name, int brackets)
{
  if (output && strlen(output) > 0){
    //check if we do have categories declared
    if (xbt_dict_is_empty(created_categories)){
      XBT_INFO("No categories declared, ignoring generation of %s graph configuration", name);
      return;
    }

    FILE *file = fopen (output, "w");
    if (file == NULL){
      THROWF (system_error, 1, "Unable to open file (%s) for writing %s graph "
          "configuration (categorized).", output, name);
    }

    if (brackets) fprintf (file, "{\n");
    cat_configuration (file);
    if (brackets) fprintf (file, "}\n");
    fclose (file);
  }
}

void TRACE_generate_viva_uncat_conf (void)
{
  generate_uncat_configuration (TRACE_get_viva_uncat_conf (), "viva", 0);
}

void TRACE_generate_viva_cat_conf (void)
{
  generate_cat_configuration (TRACE_get_viva_cat_conf(), "viva", 0);
}

static int previous_trace_state = -1;

void instr_pause_tracing (void)
{
  previous_trace_state = trace_enabled;
  if (!TRACE_is_enabled()){
    XBT_DEBUG ("Tracing is already paused, therefore do nothing.");
  }else{
    XBT_DEBUG ("Tracing is being paused.");
  }
  trace_enabled = 0;
  XBT_DEBUG ("Tracing is paused.");
}

void instr_resume_tracing (void)
{
  if (TRACE_is_enabled()){
    XBT_DEBUG ("Tracing is already running while trying to resume, therefore do nothing.");
  }else{
    XBT_DEBUG ("Tracing is being resumed.");
  }

  if (previous_trace_state != -1){
    trace_enabled = previous_trace_state;
  }else{
    trace_enabled = 1;
  }
  XBT_DEBUG ("Tracing is resumed.");
  previous_trace_state = -1;
}

#undef OPT_TRACING
#undef OPT_TRACING_PLATFORM
#undef OPT_TRACING_TOPOLOGY
#undef OPT_TRACING_SMPI
#undef OPT_TRACING_SMPI_GROUP
#undef OPT_TRACING_CATEGORIZED
#undef OPT_TRACING_UNCATEGORIZED
#undef OPT_TRACING_MSG_PROCESS
#undef OPT_TRACING_FILENAME
#undef OPT_TRACING_BUFFER
#undef OPT_TRACING_ONELINK_ONLY
#undef OPT_TRACING_DISABLE_DESTROY
#undef OPT_TRACING_BASIC
#undef OPT_TRACING_COMMENT
#undef OPT_TRACING_COMMENT_FILE
#undef OPT_VIVA_UNCAT_CONF
#undef OPT_VIVA_CAT_CONF
