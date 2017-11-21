/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "include/xbt/config.hpp"
#include "src/instr/instr_private.hpp"
#include "surf/surf.hpp"
#include <string>
#include <vector>

XBT_LOG_NEW_CATEGORY(instr, "Logging the behavior of the tracing system (used for Visualization/Analysis of simulations)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_config, instr, "Configuration");

#define OPT_TRACING_BASIC                "tracing/basic"
#define OPT_TRACING_BUFFER               "tracing/buffer"
#define OPT_TRACING_CATEGORIZED          "tracing/categorized"
#define OPT_TRACING_COMMENT_FILE         "tracing/comment-file"
#define OPT_TRACING_COMMENT              "tracing/comment"
#define OPT_TRACING_DISABLE_DESTROY      "tracing/disable-destroy"
#define OPT_TRACING_DISABLE_LINK         "tracing/disable-link"
#define OPT_TRACING_DISABLE_POWER        "tracing/disable-power"
#define OPT_TRACING_DISPLAY_SIZES        "tracing/smpi/display-sizes"
#define OPT_TRACING_FILENAME             "tracing/filename"
#define OPT_TRACING_FORMAT_TI_ONEFILE    "tracing/smpi/format/ti-one-file"
#define OPT_TRACING_FORMAT               "tracing/smpi/format"
#define OPT_TRACING_MSG_PROCESS          "tracing/msg/process"
#define OPT_TRACING_MSG_VM               "tracing/msg/vm"
#define OPT_TRACING_ONELINK_ONLY         "tracing/onelink-only"
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

static bool trace_enabled = false;
static bool trace_platform;
static bool trace_platform_topology;
static bool trace_smpi_enabled;
static bool trace_smpi_grouped;
static bool trace_smpi_computing;
static bool trace_smpi_sleeping;
static bool trace_view_internals;
static bool trace_categorized;
static bool trace_uncategorized;
static bool trace_msg_process_enabled;
static bool trace_msg_vm_enabled;
static bool trace_buffer;
static bool trace_onelink_only;
static bool trace_disable_destroy;
static bool trace_basic;
static bool trace_display_sizes = false;
static bool trace_disable_link;
static bool trace_disable_power;
static int trace_precision;

static bool trace_configured = false;
static bool trace_active     = false;

instr_fmt_type_t instr_fmt_type = instr_fmt_paje;

static void TRACE_getopts()
{
  trace_enabled             = xbt_cfg_get_boolean(OPT_TRACING);
  trace_platform            = xbt_cfg_get_boolean(OPT_TRACING_PLATFORM);
  trace_platform_topology   = xbt_cfg_get_boolean(OPT_TRACING_TOPOLOGY);
  trace_smpi_enabled        = xbt_cfg_get_boolean(OPT_TRACING_SMPI);
  trace_smpi_grouped        = xbt_cfg_get_boolean(OPT_TRACING_SMPI_GROUP);
  trace_smpi_computing      = xbt_cfg_get_boolean(OPT_TRACING_SMPI_COMPUTING);
  trace_smpi_sleeping       = xbt_cfg_get_boolean(OPT_TRACING_SMPI_SLEEPING);
  trace_view_internals      = xbt_cfg_get_boolean(OPT_TRACING_SMPI_INTERNALS);
  trace_categorized         = xbt_cfg_get_boolean(OPT_TRACING_CATEGORIZED);
  trace_uncategorized       = xbt_cfg_get_boolean(OPT_TRACING_UNCATEGORIZED);
  trace_msg_process_enabled = xbt_cfg_get_boolean(OPT_TRACING_MSG_PROCESS);
  trace_msg_vm_enabled      = xbt_cfg_get_boolean(OPT_TRACING_MSG_VM);
  trace_buffer              = xbt_cfg_get_boolean(OPT_TRACING_BUFFER);
  trace_onelink_only        = xbt_cfg_get_boolean(OPT_TRACING_ONELINK_ONLY);
  trace_disable_destroy     = xbt_cfg_get_boolean(OPT_TRACING_DISABLE_DESTROY);
  trace_basic               = xbt_cfg_get_boolean(OPT_TRACING_BASIC);
  trace_display_sizes       = xbt_cfg_get_boolean(OPT_TRACING_DISPLAY_SIZES);
  trace_disable_link        = xbt_cfg_get_boolean(OPT_TRACING_DISABLE_LINK);
  trace_disable_power       = xbt_cfg_get_boolean(OPT_TRACING_DISABLE_POWER);
  trace_precision           = xbt_cfg_get_int(OPT_TRACING_PRECISION);
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

    /* open the trace file(s) */
    std::string format = xbt_cfg_get_string(OPT_TRACING_FORMAT);
    XBT_DEBUG("Tracing format %s\n", format.c_str());
    if (format == "Paje") {
      TRACE_paje_start();
    } else if (format == "TI") {
      instr_fmt_type = instr_fmt_TI;
      TRACE_TI_start();
    }else{
      xbt_die("Unknown trace format :%s ", format.c_str());
    }

    /* activate trace */
    if (trace_active) {
      THROWF(tracing_error, 0, "Tracing is already active");
    }
    trace_active = true;
    XBT_DEBUG("Tracing is on");
  }
  return 0;
}

int TRACE_end()
{
  int retval;
  if (not trace_active) {
    retval = 1;
  } else {
    retval = 0;

    /* dump trace buffer */
    TRACE_last_timestamp_to_dump = surf_get_clock();
    TRACE_paje_dump_buffer(true);

    simgrid::instr::Type* root_type = simgrid::instr::Container::getRoot()->type_;
    /* destroy all data structures of tracing (and free) */
    delete simgrid::instr::Container::getRoot();
    delete root_type;

    /* close the trace files */
    std::string format = xbt_cfg_get_string(OPT_TRACING_FORMAT);
    XBT_DEBUG("Tracing format %s\n", format.c_str());
    if (format == "Paje") {
      TRACE_paje_end();
    } else if (format == "TI") {
      TRACE_TI_end();
    }else{
      xbt_die("Unknown trace format :%s ", format.c_str());
    }

    /* de-activate trace */
    trace_active = false;
    XBT_DEBUG("Tracing is off");
    XBT_DEBUG("Tracing system is shutdown");
  }
  return retval;
}

bool TRACE_needs_platform ()
{
  return TRACE_msg_process_is_enabled() || TRACE_msg_vm_is_enabled() || TRACE_categorized() ||
         TRACE_uncategorized() || TRACE_platform () || (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped());
}

bool TRACE_is_enabled()
{
  return trace_enabled;
}

bool TRACE_platform()
{
  return trace_platform;
}

bool TRACE_platform_topology()
{
  return trace_platform_topology;
}

bool TRACE_is_configured()
{
  return trace_configured;
}

bool TRACE_smpi_is_enabled()
{
  return (trace_smpi_enabled || TRACE_smpi_is_grouped()) && TRACE_is_enabled();
}

bool TRACE_smpi_is_grouped()
{
  return trace_smpi_grouped;
}

bool TRACE_smpi_is_computing()
{
  return trace_smpi_computing;
}

bool TRACE_smpi_is_sleeping()
{
  return trace_smpi_sleeping;
}

bool TRACE_smpi_view_internals()
{
  return trace_view_internals;
}

bool TRACE_categorized ()
{
  return trace_categorized;
}

bool TRACE_uncategorized ()
{
  return trace_uncategorized;
}

bool TRACE_msg_process_is_enabled()
{
  return trace_msg_process_enabled && TRACE_is_enabled();
}

bool TRACE_msg_vm_is_enabled()
{
  return trace_msg_vm_enabled && TRACE_is_enabled();
}

bool TRACE_disable_link()
{
  return trace_disable_link && TRACE_is_enabled();
}

bool TRACE_disable_speed()
{
  return trace_disable_power && TRACE_is_enabled();
}

bool TRACE_buffer ()
{
  return trace_buffer && TRACE_is_enabled();
}

bool TRACE_onelink_only ()
{
  return trace_onelink_only && TRACE_is_enabled();
}

bool TRACE_disable_destroy ()
{
  return trace_disable_destroy && TRACE_is_enabled();
}

bool TRACE_basic ()
{
  return trace_basic && TRACE_is_enabled();
}

bool TRACE_display_sizes ()
{
   return trace_display_sizes && trace_smpi_enabled && TRACE_is_enabled();
}

std::string TRACE_get_comment()
{
  return xbt_cfg_get_string(OPT_TRACING_COMMENT);
}

std::string TRACE_get_comment_file()
{
  return xbt_cfg_get_string(OPT_TRACING_COMMENT_FILE);
}

int TRACE_precision ()
{
  return xbt_cfg_get_int(OPT_TRACING_PRECISION);
}

std::string TRACE_get_filename()
{
  return xbt_cfg_get_string(OPT_TRACING_FILENAME);
}

void TRACE_global_init()
{
  static bool is_initialised = false;
  if (is_initialised)
    return;

  is_initialised = true;
  /* name of the tracefile */
  xbt_cfg_register_string (OPT_TRACING_FILENAME, "simgrid.trace", nullptr, "Trace file created by the instrumented SimGrid.");
  xbt_cfg_register_boolean(OPT_TRACING, "no", nullptr, "Enable Tracing.");
  xbt_cfg_register_boolean(OPT_TRACING_PLATFORM, "no", nullptr, "Register the platform in the trace as a hierarchy.");
  xbt_cfg_register_boolean(OPT_TRACING_TOPOLOGY, "yes", nullptr, "Register the platform topology in the trace as a graph.");
  xbt_cfg_register_boolean(OPT_TRACING_SMPI, "no", nullptr, "Tracing of the SMPI interface.");
  xbt_cfg_register_boolean(OPT_TRACING_SMPI_GROUP,"no", nullptr, "Group MPI processes by host.");
  xbt_cfg_register_boolean(OPT_TRACING_SMPI_COMPUTING, "no", nullptr, "Generate states for timing out of SMPI parts of the application");
  xbt_cfg_register_boolean(OPT_TRACING_SMPI_SLEEPING, "no", nullptr, "Generate states for timing out of SMPI parts of the application");
  xbt_cfg_register_boolean(OPT_TRACING_SMPI_INTERNALS, "no", nullptr, "View internal messages sent by Collective communications in SMPI");
  xbt_cfg_register_boolean(OPT_TRACING_CATEGORIZED, "no", nullptr, "Tracing categorized resource utilization of hosts and links.");
  xbt_cfg_register_boolean(OPT_TRACING_UNCATEGORIZED, "no", nullptr, "Tracing uncategorized resource utilization of hosts and links.");

  xbt_cfg_register_boolean(OPT_TRACING_MSG_PROCESS, "no", nullptr, "Tracing of MSG process behavior.");
  xbt_cfg_register_boolean(OPT_TRACING_MSG_VM, "no", nullptr, "Tracing of MSG process behavior.");
  xbt_cfg_register_boolean(OPT_TRACING_DISABLE_LINK, "no", nullptr, "Do not trace link bandwidth and latency.");
  xbt_cfg_register_boolean(OPT_TRACING_DISABLE_POWER, "no", nullptr, "Do not trace host power.");
  xbt_cfg_register_boolean(OPT_TRACING_BUFFER, "yes", nullptr, "Buffer trace events to put them in temporal order.");

  xbt_cfg_register_boolean(OPT_TRACING_ONELINK_ONLY, "no", nullptr, "Use only routes with one link to trace platform.");
  xbt_cfg_register_boolean(OPT_TRACING_DISABLE_DESTROY, "no", nullptr, "Disable platform containers destruction.");
  xbt_cfg_register_boolean(OPT_TRACING_BASIC, "no", nullptr, "Avoid extended events (impoverished trace file).");
  xbt_cfg_register_boolean(OPT_TRACING_DISPLAY_SIZES, "no", nullptr, "(smpi only) Extended events with message size information");
  xbt_cfg_register_string(OPT_TRACING_FORMAT, "Paje", nullptr, "(smpi only) Switch the output format of Tracing");
  xbt_cfg_register_boolean(OPT_TRACING_FORMAT_TI_ONEFILE, "no", nullptr, "(smpi only) For replay format only : output to one file only");
  xbt_cfg_register_string(OPT_TRACING_COMMENT, "", nullptr, "Comment to be added on the top of the trace file.");
  xbt_cfg_register_string(OPT_TRACING_COMMENT_FILE, "", nullptr,
      "The contents of the file are added to the top of the trace file as comment.");
  xbt_cfg_register_int(OPT_TRACING_PRECISION, 6, nullptr, "Numerical precision used when timestamping events "
      "(expressed in number of digits after decimal point)");

  xbt_cfg_register_alias(OPT_TRACING_COMMENT_FILE,"tracing/comment_file");
  xbt_cfg_register_alias(OPT_TRACING_DISABLE_DESTROY, "tracing/disable_destroy");
  xbt_cfg_register_alias(OPT_TRACING_DISABLE_LINK, "tracing/disable_link");
  xbt_cfg_register_alias(OPT_TRACING_DISABLE_POWER, "tracing/disable_power");
  xbt_cfg_register_alias(OPT_TRACING_DISPLAY_SIZES, "tracing/smpi/display_sizes");
  xbt_cfg_register_alias(OPT_TRACING_FORMAT_TI_ONEFILE, "tracing/smpi/format/ti_one_file");
  xbt_cfg_register_alias(OPT_TRACING_ONELINK_ONLY, "tracing/onelink_only");

  /* instrumentation can be considered configured now */
  trace_configured = true;
}

static void print_line (const char *option, const char *desc, const char *longdesc, int detailed)
{
  std::string str = std::string("--cfg=") + option + " ";

  int len = str.size();
  printf("%s%*.*s %s\n", str.c_str(), 30 - len, 30 - len, "", desc);
  if (longdesc != nullptr && detailed){
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
  print_line(OPT_TRACING_FILENAME, "Filename to register traces",
             "  A file with this name will be created to register the simulation. The file\n"
             "  is in the Paje format and can be analyzed using Paje, and PajeNG visualization\n"
             "  tools. More information can be found in these webpages:\n"
             "     http://github.com/schnorr/pajeng/\n"
             "     http://paje.sourceforge.net/",
             detailed);
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
  print_line (OPT_TRACING_TOPOLOGY, "Register the platform topology as a graph",
        "  This option (enabled by default) can be used to disable the tracing of\n"
        "  the platform topology in the trace file. Sometimes, such task is really\n"
        "  time consuming, since it must get the route from each host to other hosts\n"
        "  within the same Autonomous System (AS).", detailed);
}

static void output_types (const char *name, xbt_dynar_t types, FILE *file)
{
  unsigned int i;
  fprintf (file, "  %s = (", name);
  for (i = xbt_dynar_length(types); i > 0; i--) {
    char *type = *(static_cast<char**>(xbt_dynar_get_ptr(types, i - 1)));
    fprintf (file, "\"%s\"", type);
    if (i - 1 > 0){
      fprintf (file, ",");
    }else{
      fprintf (file, ");\n");
    }
  }
  xbt_dynar_free (&types);
}

static int previous_trace_state = -1;

void instr_pause_tracing ()
{
  previous_trace_state = trace_enabled;
  if (not TRACE_is_enabled()) {
    XBT_DEBUG ("Tracing is already paused, therefore do nothing.");
  }else{
    XBT_DEBUG ("Tracing is being paused.");
  }
  trace_enabled = false;
  XBT_DEBUG ("Tracing is paused.");
}

void instr_resume_tracing ()
{
  if (TRACE_is_enabled()){
    XBT_DEBUG ("Tracing is already running while trying to resume, therefore do nothing.");
  }else{
    XBT_DEBUG ("Tracing is being resumed.");
  }

  if (previous_trace_state != -1){
    trace_enabled = previous_trace_state;
  }else{
    trace_enabled = true;
  }
  XBT_DEBUG ("Tracing is resumed.");
  previous_trace_state = -1;
}
