/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "include/xbt/config.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/version.h"
#include "src/instr/instr_private.hpp"
#include "surf/surf.hpp"
#include "xbt/virtu.h" /* xbt_cmdline */

#include <fstream>
#include <string>
#include <vector>

XBT_LOG_NEW_CATEGORY(instr, "Logging the behavior of the tracing system (used for Visualization/Analysis of simulations)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_config, instr, "Configuration");

std::ofstream tracing_file;

constexpr char OPT_TRACING_BASIC[]             = "tracing/basic";
constexpr char OPT_TRACING_COMMENT_FILE[]      = "tracing/comment-file";
constexpr char OPT_TRACING_DISABLE_DESTROY[]   = "tracing/disable-destroy";
constexpr char OPT_TRACING_FORMAT_TI_ONEFILE[] = "tracing/smpi/format/ti-one-file";
constexpr char OPT_TRACING_SMPI[]              = "tracing/smpi";
constexpr char OPT_TRACING_TOPOLOGY[]          = "tracing/platform/topology";

static simgrid::config::Flag<bool> trace_enabled{
    "tracing", "Enable the tracing system. You have to enable this option to use other tracing options.", false};

static simgrid::config::Flag<bool> trace_actor_enabled{
    "tracing/msg/process", // FIXME rename this flag
    "Trace the behavior of all categorized actors, grouping them by host. "
    "Can be used to track actor location if the simulator does actor migration.",
    false};

static simgrid::config::Flag<bool> trace_vm_enabled{"tracing/vm", "Trace the behavior of all virtual machines.", false};

static simgrid::config::Flag<bool> trace_platform{"tracing/platform",
                                                  "Register the platform in the trace as a hierarchy.", false};

static simgrid::config::Flag<bool> trace_platform_topology{
    OPT_TRACING_TOPOLOGY, "Register the platform topology in the trace as a graph.", true};
static simgrid::config::Flag<bool> trace_smpi_enabled{OPT_TRACING_SMPI, "Tracing of the SMPI interface.", false};
static simgrid::config::Flag<bool> trace_smpi_grouped{"tracing/smpi/group", "Group MPI processes by host.", false};

static simgrid::config::Flag<bool> trace_smpi_computing{
    "tracing/smpi/computing", "Generate 'Computing' states to trace the out-of-SMPI parts of the application", false};

static simgrid::config::Flag<bool> trace_smpi_sleeping{
    "tracing/smpi/sleeping", "Generate 'Sleeping' states for the sleeps in the application that do not pertain to SMPI",
    false};

static simgrid::config::Flag<bool> trace_view_internals{
    "tracing/smpi/internals",
    "Generate tracing events corresponding to point-to-point messages sent by SMPI collective communications", false};

static simgrid::config::Flag<bool> trace_categorized{
    "tracing/categorized", "Trace categorized resource utilization of hosts and links.", false};

static simgrid::config::Flag<bool> trace_uncategorized{
    "tracing/uncategorized",
    "Trace uncategorized resource utilization of hosts and links. "
    "To use if the simulator does not use tracing categories but resource utilization have to be traced.",
    false};

static simgrid::config::Flag<bool> trace_disable_destroy{
    OPT_TRACING_DISABLE_DESTROY, {"tracing/disable_destroy"}, "Disable platform containers destruction.", false};
static simgrid::config::Flag<bool> trace_basic{OPT_TRACING_BASIC, "Avoid extended events (impoverished trace file).",
                                               false};

static simgrid::config::Flag<bool> trace_display_sizes{
    "tracing/smpi/display-sizes",
    "Add message size information (in bytes) to the to links and states (SMPI only). "
    "For collectives, it usually corresponds to the total number of bytes sent by a process.",
    false};

static simgrid::config::Flag<bool> trace_disable_link{"tracing/disable_link",
                                                      "Do not trace link bandwidth and latency.", false};
static simgrid::config::Flag<bool> trace_disable_power{"tracing/disable_power", "Do not trace host power.", false};

static bool trace_active     = false;

simgrid::instr::TraceFormat simgrid::instr::trace_format = simgrid::instr::TraceFormat::Paje;

static void TRACE_start()
{
  if (trace_active)
    return;

  // tracing system must be:
  //    - enabled (with --cfg=tracing:yes)
  //    - already configured (TRACE_global_init already called)
  if (TRACE_is_enabled()) {
    instr_define_callbacks();

    XBT_DEBUG("Tracing starts");
    /* init the tracing module to generate the right output */
    std::string format = simgrid::config::get_value<std::string>("tracing/smpi/format");
    XBT_DEBUG("Tracing format %s", format.c_str());

    /* open the trace file(s) */
    std::string filename = TRACE_get_filename();
    tracing_file.open(filename.c_str(), std::ofstream::out);
    if (tracing_file.fail()) {
      throw simgrid::TracingError(
          XBT_THROW_POINT,
          simgrid::xbt::string_printf("Tracefile %s could not be opened for writing.", filename.c_str()));
    }

    XBT_DEBUG("Filename %s is open for writing", filename.c_str());

    if (format == "Paje") {
      /* output generator version */
      tracing_file << "#This file was generated using SimGrid-" << SIMGRID_VERSION_MAJOR << "." << SIMGRID_VERSION_MINOR
                   << "." << SIMGRID_VERSION_PATCH << std::endl;
      tracing_file << "#[";
      unsigned int cpt;
      char* str;
      xbt_dynar_foreach (xbt_cmdline, cpt, str) {
        tracing_file << str << " ";
      }
      tracing_file << "]" << std::endl;
    }

    /* output one line comment */
    dump_comment(simgrid::config::get_value<std::string>("tracing/comment"));

    /* output comment file */
    dump_comment_file(simgrid::config::get_value<std::string>(OPT_TRACING_COMMENT_FILE));

    if (format == "Paje") {
      /* output Pajé header */
      TRACE_header(TRACE_basic(), TRACE_display_sizes());
    } else
      simgrid::instr::trace_format = simgrid::instr::TraceFormat::Ti;

    trace_active = true;
    XBT_DEBUG("Tracing is on");
  }
}

static void TRACE_end()
{
  if (not trace_active)
    return;

  /* dump trace buffer */
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(true);

  simgrid::instr::Type* root_type = simgrid::instr::Container::get_root()->type_;
  /* destroy all data structures of tracing (and free) */
  delete simgrid::instr::Container::get_root();
  delete root_type;

  /* close the trace files */
  tracing_file.close();
  XBT_DEBUG("Filename %s is closed", TRACE_get_filename().c_str());

  /* de-activate trace */
  trace_active = false;
  XBT_DEBUG("Tracing is off");
  XBT_DEBUG("Tracing system is shutdown");
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
  return trace_actor_enabled && trace_enabled;
}

bool TRACE_vm_is_enabled()
{
  return trace_vm_enabled && trace_enabled;
}

bool TRACE_disable_link()
{
  return trace_disable_link && trace_enabled;
}

bool TRACE_disable_speed()
{
  return trace_disable_power && trace_enabled;
}

bool TRACE_disable_destroy ()
{
  return trace_disable_destroy && trace_enabled;
}

bool TRACE_basic ()
{
  return trace_basic && trace_enabled;
}

bool TRACE_display_sizes ()
{
  return trace_display_sizes && trace_smpi_enabled && trace_enabled;
}

int TRACE_precision ()
{
  return simgrid::config::get_value<int>("tracing/precision");
}

std::string TRACE_get_filename()
{
  return simgrid::config::get_value<std::string>("tracing/filename");
}

void TRACE_global_init()
{
  static bool is_initialized = false;
  if (is_initialized)
    return;

  is_initialized = true;

  /* name of the tracefile */
  simgrid::config::declare_flag<std::string>("tracing/filename", "Trace file created by the instrumented SimGrid.",
                                             "simgrid.trace");
  simgrid::config::declare_flag<std::string>(
      "tracing/smpi/format", "Select trace output format used by SMPI. The default is the 'Paje' format. "
                             "The 'TI' (Time-Independent) format allows for trace replay.",
      "Paje");

  simgrid::config::declare_flag<bool>(OPT_TRACING_FORMAT_TI_ONEFILE,
                                      "(smpi only) For replay format only : output to one file only", false);
  simgrid::config::alias(OPT_TRACING_FORMAT_TI_ONEFILE, {"tracing/smpi/format/ti_one_file"});
  simgrid::config::declare_flag<std::string>("tracing/comment", "Add a comment line to the top of the trace file.", "");
  simgrid::config::declare_flag<std::string>(OPT_TRACING_COMMENT_FILE,
                                             "Add the contents of a file as comments to the top of the trace.", "");
  simgrid::config::alias(OPT_TRACING_COMMENT_FILE, {"tracing/comment_file"});
  simgrid::config::declare_flag<int>("tracing/precision", "Numerical precision used when timestamping events "
                                                          "(expressed in number of digits after decimal point)",
                                     6);

  /* Connect callbacks */
  simgrid::s4u::Engine::on_platform_creation.connect(TRACE_start);
  simgrid::s4u::Engine::on_deadlock.connect(TRACE_end);
  simgrid::s4u::Engine::on_simulation_end.connect(TRACE_end);
}

static void print_line(const char* option, const char* desc, const char* longdesc)
{
  std::string str = std::string("--cfg=") + option + " ";

  int len = str.size();
  XBT_HELP("%s%*.*s %s", str.c_str(), 30 - len, 30 - len, "", desc);
  if (longdesc != nullptr) {
    XBT_HELP("%s\n", longdesc);
  }
}

void TRACE_help()
{
  XBT_HELP("Description of the tracing options accepted by this simulator:\n");
  print_line(OPT_TRACING_SMPI, "Trace the MPI Interface (SMPI)",
             "  This option only has effect if this simulator is SMPI-based. Traces the MPI\n"
             "  interface and generates a trace that can be analyzed using Gantt-like\n"
             "  visualizations. Every MPI function (implemented by SMPI) is transformed in a\n"
             "  state, and point-to-point communications can be analyzed with arrows.");
  print_line(OPT_TRACING_DISABLE_DESTROY, "Disable platform containers destruction",
             "  Disable the destruction of containers at the end of simulation. This can be\n"
             "  used with simulators that have a different notion of time (different from\n"
             "  the simulated time).");
  print_line(OPT_TRACING_BASIC, "Avoid extended events (impoverished trace file).",
             "  Some visualization tools are not able to parse correctly the Paje file format.\n"
             "  Use this option if you are using one of these tools to visualize the simulation\n"
             "  trace. Keep in mind that the trace might be incomplete, without all the\n"
             "  information that would be registered otherwise.");
  print_line(OPT_TRACING_FORMAT_TI_ONEFILE, "Only works for SMPI now, and TI output format",
             "  By default, each process outputs to a separate file, inside a filename_files folder\n"
             "  By setting this option to yes, all processes will output to only one file\n"
             "  This is meant to avoid opening thousands of files with large simulations");
  print_line(OPT_TRACING_TOPOLOGY, "Register the platform topology as a graph",
             "  This option (enabled by default) can be used to disable the tracing of\n"
             "  the platform topology in the trace file. Sometimes, such task is really\n"
             "  time consuming, since it must get the route from each host to other hosts\n"
             "  within the same Autonomous System (AS).");
}
