/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

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
#define OPT_TRACING_ACTOR "tracing/msg/process"
#define OPT_TRACING_VM "tracing/vm"
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

static simgrid::config::Flag<bool> trace_enabled{OPT_TRACING, "Enable Tracing.", false};
static simgrid::config::Flag<bool> trace_platform{OPT_TRACING_PLATFORM,
                                                  "Register the platform in the trace as a hierarchy.", false};
static simgrid::config::Flag<bool> trace_platform_topology{
    OPT_TRACING_TOPOLOGY, "Register the platform topology in the trace as a graph.", true};
static simgrid::config::Flag<bool> trace_smpi_enabled{OPT_TRACING_SMPI, "Tracing of the SMPI interface.", false};
static simgrid::config::Flag<bool> trace_smpi_grouped{OPT_TRACING_SMPI_GROUP, "Group MPI processes by host.", false};
static simgrid::config::Flag<bool> trace_smpi_computing{
    OPT_TRACING_SMPI_COMPUTING, "Generate states for timing out of SMPI parts of the application", false};
static simgrid::config::Flag<bool> trace_smpi_sleeping{
    OPT_TRACING_SMPI_SLEEPING, "Generate states for timing out of SMPI parts of the application", false};
static simgrid::config::Flag<bool> trace_view_internals{
    OPT_TRACING_SMPI_INTERNALS, "View internal messages sent by Collective communications in SMPI", false};
static simgrid::config::Flag<bool> trace_categorized{
    OPT_TRACING_CATEGORIZED, "Tracing categorized resource utilization of hosts and links.", false};
static simgrid::config::Flag<bool> trace_uncategorized{
    OPT_TRACING_UNCATEGORIZED, "Tracing uncategorized resource utilization of hosts and links.", false};
static simgrid::config::Flag<bool> trace_actor_enabled{OPT_TRACING_ACTOR, "Tracing of actor behavior.", false};
static simgrid::config::Flag<bool> trace_vm_enabled{OPT_TRACING_VM, "Tracing of virtual machine behavior.", false};
static simgrid::config::Flag<bool> trace_buffer{OPT_TRACING_BUFFER,
                                                "Buffer trace events to put them in temporal order.", true};
static simgrid::config::Flag<bool> trace_disable_destroy{
    OPT_TRACING_DISABLE_DESTROY, {"tracing/disable_destroy"}, "Disable platform containers destruction.", false};
static simgrid::config::Flag<bool> trace_basic{OPT_TRACING_BASIC, "Avoid extended events (impoverished trace file).",
                                               false};
static simgrid::config::Flag<bool> trace_display_sizes{OPT_TRACING_DISPLAY_SIZES,
                                                       {"tracing/smpi/display_sizes"},
                                                       "(smpi only) Extended events with message size information",
                                                       false};
static simgrid::config::Flag<bool> trace_disable_link{
    OPT_TRACING_DISABLE_LINK, {"tracing/disable_link"}, "Do not trace link bandwidth and latency.", false};
static simgrid::config::Flag<bool> trace_disable_power{
    OPT_TRACING_DISABLE_POWER, {"tracing/disable_power"}, "Do not trace host power.", false};

static bool trace_active     = false;

simgrid::instr::TraceFormat simgrid::instr::trace_format = simgrid::instr::TraceFormat::Paje;

int TRACE_start()
{
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
      simgrid::instr::trace_format = simgrid::instr::TraceFormat::Ti;
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
  return TRACE_actor_is_enabled() || TRACE_vm_is_enabled() || TRACE_categorized() || TRACE_uncategorized() ||
         TRACE_platform() || (TRACE_smpi_is_enabled() && TRACE_smpi_is_grouped());
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

bool TRACE_actor_is_enabled()
{
  return trace_actor_enabled && TRACE_is_enabled();
}

bool TRACE_vm_is_enabled()
{
  return trace_vm_enabled && TRACE_is_enabled();
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
  simgrid::config::declare_flag<std::string>(OPT_TRACING_FILENAME, "Trace file created by the instrumented SimGrid.",
                                             "simgrid.trace");
  simgrid::config::declare_flag<std::string>(OPT_TRACING_FORMAT, "(smpi only) Switch the output format of Tracing",
                                             "Paje");
  simgrid::config::declare_flag<bool>(OPT_TRACING_FORMAT_TI_ONEFILE,
                                      "(smpi only) For replay format only : output to one file only", false);
  simgrid::config::alias(OPT_TRACING_FORMAT_TI_ONEFILE, {"tracing/smpi/format/ti_one_file"});
  simgrid::config::declare_flag<std::string>(OPT_TRACING_COMMENT, "Comment to be added on the top of the trace file.",
                                             "");
  simgrid::config::declare_flag<std::string>(
      OPT_TRACING_COMMENT_FILE, "The contents of the file are added to the top of the trace file as comment.", "");
  simgrid::config::alias(OPT_TRACING_COMMENT_FILE, {"tracing/comment_file"});
  simgrid::config::declare_flag<int>(OPT_TRACING_PRECISION, "Numerical precision used when timestamping events "
                                                            "(expressed in number of digits after decimal point)",
                                     6);
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
  print_line(OPT_TRACING_ACTOR, "Trace actor behavior",
             "  This option traces the behavior of all categorized actors, grouping them\n"
             "  by hosts. This option can be used to track actor location if the simulator\n"
             "  does actor migration.",
             detailed);
  print_line (OPT_TRACING_BUFFER, "Buffer events to put them in temporal order",
      "  This option put some events in a time-ordered buffer using the insertion\n"
      "  sort algorithm. The process of acquiring and releasing locks to access this\n"
      "  buffer and the cost of the sorting algorithm make this process slow. The\n"
      "  simulator performance can be severely impacted if this option is activated,\n"
      "  but you are sure to get a trace file with events sorted.", detailed);
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
