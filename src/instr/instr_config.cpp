/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "include/xbt/config.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/version.h"
#include "src/instr/instr_private.hpp"
#include "surf/surf.hpp"
#include "xbt/virtu.h" /* xbt::cmdline */

#include <sys/stat.h>
#ifdef WIN32
#include <direct.h> // _mkdir
#endif

#include <fstream>
#include <string>
#include <vector>

XBT_LOG_NEW_CATEGORY(instr, "Logging the behavior of the tracing system (used for Visualization/Analysis of simulations)");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_config, instr, "Configuration");

std::ofstream tracing_file;
std::map<container_t, std::ofstream*> tracing_files; // TI specific
double prefix = 0.0;                                 // TI specific

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

simgrid::instr::TraceFormat simgrid::instr::trace_format = simgrid::instr::TraceFormat::Paje;

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

namespace simgrid {
namespace instr {

static bool trace_active = false;
/*************
 * Callbacks *
 *************/
xbt::signal<void(Container&)> Container::on_creation;
xbt::signal<void(Container&)> Container::on_destruction;
xbt::signal<void(EntityValue&)> EntityValue::on_creation;
xbt::signal<void(Type&, e_event_type)> Type::on_creation;
xbt::signal<void(LinkType&, Type&, Type&)> LinkType::on_creation;

static void on_container_creation_paje(Container& c)
{
  double timestamp = SIMIX_get_clock();
  std::stringstream stream;

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, PAJE_CreateContainer, timestamp);

  stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_CreateContainer << " ";
  stream << timestamp << " " << c.get_id() << " " << c.type_->get_id() << " " << c.father_->get_id() << " \"";
  if (c.get_name().find("rank-") != 0)
    stream << c.get_name() << "\"";
  else
    /* Subtract -1 because this is the process id and we transform it to the rank id */
    stream << "rank-" << stoi(c.get_name().substr(5)) - 1 << "\"";

  XBT_DEBUG("Dump %s", stream.str().c_str());
  tracing_file << stream.str() << std::endl;
}

static void on_container_destruction_paje(Container& c)
{
  // obligation to dump previous events because they might reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = SIMIX_get_clock();
  TRACE_paje_dump_buffer(true);

  // trace my destruction, but not if user requests so or if the container is root
  if (not trace_disable_destroy && &c != Container::get_root()) {
    std::stringstream stream;
    double timestamp = SIMIX_get_clock();

    XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, PAJE_DestroyContainer, timestamp);

    stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_DestroyContainer << " ";
    stream << timestamp << " " << c.type_->get_id() << " " << c.get_id();
    XBT_DEBUG("Dump %s", stream.str().c_str());
    tracing_file << stream.str() << std::endl;
  }
}

static void on_container_creation_ti(Container& c)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, PAJE_CreateContainer, SIMIX_get_clock());
  // if we are in the mode with only one file
  static std::ofstream* ti_unique_file = nullptr;

  if (tracing_files.empty()) {
    // generate unique run id with time
    prefix = xbt_os_time();
  }

  if (not simgrid::config::get_value<bool>("tracing/smpi/format/ti-one-file") || ti_unique_file == nullptr) {
    std::string folder_name = simgrid::config::get_value<std::string>("tracing/filename") + "_files";
    std::string filename    = folder_name + "/" + std::to_string(prefix) + "_" + c.get_name() + ".txt";
#ifdef WIN32
    _mkdir(folder_name.c_str());
#else
    mkdir(folder_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    ti_unique_file = new std::ofstream(filename.c_str(), std::ofstream::out);
    xbt_assert(not ti_unique_file->fail(), "Tracefile %s could not be opened for writing", filename.c_str());
    tracing_file << filename << std::endl;
  }
  tracing_files.insert({&c, ti_unique_file});
}

static void on_container_destruction_ti(Container& c)
{
  // obligation to dump previous events because they might reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = SIMIX_get_clock();
  TRACE_paje_dump_buffer(true);

  if (not trace_disable_destroy && &c != Container::get_root()) {
    XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, PAJE_DestroyContainer, SIMIX_get_clock());
    if (not simgrid::config::get_value<bool>("tracing/smpi/format/ti-one-file") || tracing_files.size() == 1) {
      tracing_files.at(&c)->close();
      delete tracing_files.at(&c);
    }
    tracing_files.erase(&c);
  }
}

static void on_entity_value_creation(EntityValue& value)
{
  std::stringstream stream;
  XBT_DEBUG("%s: event_type=%u", __func__, PAJE_DefineEntityValue);
  stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_DefineEntityValue;
  stream << " " << value.get_id() << " " << value.get_father()->get_id() << " " << value.get_name();
  if (not value.get_color().empty())
    stream << " \"" << value.get_color() << "\"";
  XBT_DEBUG("Dump %s", stream.str().c_str());
  tracing_file << stream.str() << std::endl;
}

static void on_type_creation(Type& type, e_event_type event_type)
{
  if (event_type == PAJE_DefineLinkType)
    return; // this kind of type has to be handled differently

  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, event_type, TRACE_precision(), 0.);
  stream << event_type << " " << type.get_id() << " " << type.get_father()->get_id() << " " << type.get_name();
  if (type.is_colored())
    stream << " \"" << type.get_color() << "\"";
  XBT_DEBUG("Dump %s", stream.str().c_str());
  tracing_file << stream.str() << std::endl;
}

static void on_link_type_creation(Type& type, Type& source, Type& dest)
{
  std::stringstream stream;
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, PAJE_DefineLinkType, TRACE_precision(), 0.);
  stream << PAJE_DefineLinkType << " " << type.get_id() << " " << type.get_father()->get_id();
  stream << " " << source.get_id() << " " << dest.get_id() << " " << type.get_name();
  XBT_DEBUG("Dump %s", stream.str().c_str());
  tracing_file << stream.str() << std::endl;
}
static void on_simulation_start()
{
  if (trace_active)
    return;

  // tracing system must be:
  //    - enabled (with --cfg=tracing:yes)
  //    - already configured (simgrid::instr::init already called)
  if (TRACE_is_enabled()) {
    define_callbacks();

    XBT_DEBUG("Tracing starts");

    /* init the tracing module to generate the right output */
    std::string format = config::get_value<std::string>("tracing/smpi/format");
    XBT_DEBUG("Tracing format %s", format.c_str());

    /* Connect the callbacks associated to the creation/destruction of containers*/
    if (format == "Paje") {
      Container::on_creation.connect(on_container_creation_paje);
      Container::on_destruction.connect(on_container_destruction_paje);
      EntityValue::on_creation.connect(on_entity_value_creation);
      Type::on_creation.connect(on_type_creation);
      LinkType::on_creation.connect(on_link_type_creation);
    } else {
      Container::on_creation.connect(on_container_creation_ti);
      Container::on_destruction.connect(on_container_destruction_ti);
    }

    /* open the trace file(s) */
    std::string filename = TRACE_get_filename();
    tracing_file.open(filename.c_str(), std::ofstream::out);
    if (tracing_file.fail()) {
      throw TracingError(XBT_THROW_POINT,
                         xbt::string_printf("Tracefile %s could not be opened for writing.", filename.c_str()));
    }

    XBT_DEBUG("Filename %s is open for writing", filename.c_str());

    if (format == "Paje") {
      /* output generator version */
      tracing_file << "#This file was generated using SimGrid-" << SIMGRID_VERSION_MAJOR << "." << SIMGRID_VERSION_MINOR
                   << "." << SIMGRID_VERSION_PATCH << std::endl;
      tracing_file << "#[";
      for (auto str : simgrid::xbt::cmdline) {
        tracing_file << str << " ";
      }
      tracing_file << "]" << std::endl;
    }

    /* output one line comment */
    std::string comment = simgrid::config::get_value<std::string>("tracing/comment");
    if (not comment.empty())
      tracing_file << "# " << comment << std::endl;

    /* output comment file */
    dump_comment_file(simgrid::config::get_value<std::string>(OPT_TRACING_COMMENT_FILE));

    if (format == "Paje") {
      /* output Pajé header */
      TRACE_header(TRACE_basic(), TRACE_display_sizes());
    } else
      trace_format = TraceFormat::Ti;

    trace_active = true;
    XBT_DEBUG("Tracing is on");
  }
}

static void on_simulation_end()
{
  if (not trace_active)
    return;

  /* dump trace buffer */
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(true);

  const Type* root_type = Container::get_root()->type_;
  /* destroy all data structures of tracing (and free) */
  delete Container::get_root();
  delete root_type;

  /* close the trace files */
  tracing_file.close();
  XBT_DEBUG("Filename %s is closed", TRACE_get_filename().c_str());

  /* de-activate trace */
  trace_active = false;
  XBT_DEBUG("Tracing is off");
  XBT_DEBUG("Tracing system is shutdown");
}

void init()
{
  static bool is_initialized = false;
  if (is_initialized)
    return;

  is_initialized = true;

  /* name of the tracefile */
  config::declare_flag<std::string>("tracing/filename", "Trace file created by the instrumented SimGrid.",
                                    "simgrid.trace");
  config::declare_flag<std::string>("tracing/smpi/format",
                                    "Select trace output format used by SMPI. The default is the 'Paje' format. "
                                    "The 'TI' (Time-Independent) format allows for trace replay.",
                                    "Paje");

  config::declare_flag<bool>(OPT_TRACING_FORMAT_TI_ONEFILE,
                             "(smpi only) For replay format only : output to one file only", false);
  config::alias(OPT_TRACING_FORMAT_TI_ONEFILE, {"tracing/smpi/format/ti_one_file"});
  config::declare_flag<std::string>("tracing/comment", "Add a comment line to the top of the trace file.", "");
  config::declare_flag<std::string>(OPT_TRACING_COMMENT_FILE,
                                    "Add the contents of a file as comments to the top of the trace.", "");
  config::alias(OPT_TRACING_COMMENT_FILE, {"tracing/comment_file"});
  config::declare_flag<int>("tracing/precision",
                            "Numerical precision used when timestamping events "
                            "(expressed in number of digits after decimal point)",
                            6);

  /* Connect callbacks */
  s4u::Engine::on_platform_creation.connect(on_simulation_start);
  s4u::Engine::on_deadlock.connect(on_simulation_end);
  s4u::Engine::on_simulation_end.connect(on_simulation_end);
}
} // namespace instr
} // namespace simgrid
