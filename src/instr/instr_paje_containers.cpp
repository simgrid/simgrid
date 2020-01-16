/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"
#include <sys/stat.h>
#ifdef WIN32
#include <direct.h> // _mkdir
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

extern std::ofstream tracing_file;
std::map<container_t, std::ofstream*> tracing_files; // TI specific
double prefix = 0.0;                               // TI specific

static container_t rootContainer = nullptr;    /* the root container */
static std::map<std::string, container_t> allContainers; /* all created containers indexed by name */

long long int instr_new_paje_id ()
{
  static long long int type_id = 0;
  return type_id++;
}

namespace simgrid {
namespace instr {

container_t Container::get_root()
{
  return rootContainer;
}

NetZoneContainer::NetZoneContainer(const std::string& name, unsigned int level, NetZoneContainer* father)
    : Container::Container(name, "", father)
{
  netpoint_ = simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name());
  xbt_assert(netpoint_, "Element '%s' not found", get_cname());
  if (father_) {
    std::string type_name = std::string("L") + std::to_string(level);
    type_                 = father_->type_->by_name_or_create<ContainerType>(type_name);
    father_->children_.insert({get_name(), this});
    log_creation();
  } else {
    type_         = new ContainerType("0");
    rootContainer = this;
  }
}

RouterContainer::RouterContainer(const std::string& name, Container* father)
    : Container::Container(name, "ROUTER", father)
{
  xbt_assert(father, "Only the Root container has no father");

  netpoint_ = simgrid::s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name());
  xbt_assert(netpoint_, "Element '%s' not found", get_cname());
}

HostContainer::HostContainer(simgrid::s4u::Host const& host, NetZoneContainer* father)
    : Container::Container(host.get_name(), "HOST", father)
{
  xbt_assert(father, "Only the Root container has no father");

  netpoint_ = host.get_netpoint();
  xbt_assert(netpoint_, "Element '%s' not found", host.get_cname());
}

Container::Container(const std::string& name, const std::string& type_name, Container* father)
    : name_(name), father_(father)
{
  static long long int container_id = 0;
  id_                               = container_id; // id (or alias) of the container
  container_id++;

  if (father_) {
    XBT_DEBUG("new container %s, child of %s", get_cname(), father->get_cname());

    if (not type_name.empty()) {
      type_ = father_->type_->by_name_or_create<ContainerType>(type_name);
      father_->children_.insert({name_, this});
      log_creation();
    }
  }

  //register all kinds by name
  if (not allContainers.emplace(name_, this).second)
    throw simgrid::TracingError(
        XBT_THROW_POINT,
        simgrid::xbt::string_printf("container %s already present in allContainers data structure", get_cname()));

  XBT_DEBUG("Add container name '%s'", get_cname());
}

Container::~Container()
{
  XBT_DEBUG("destroy container %s", get_cname());
  // Begin with destroying my own children
  for (auto child : children_)
    delete child.second;

  // obligation to dump previous events because they might reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = SIMIX_get_clock();
  TRACE_paje_dump_buffer(true);

  // trace my destruction, but not if user requests so or if the container is root
  if (not TRACE_disable_destroy() && this != Container::get_root())
    log_destruction();

  // remove me from the allContainers data structure
  allContainers.erase(name_);
}

void Container::create_child(const std::string& name, const std::string& type_name)
{
  new Container(name, type_name, this);
}

Container* Container::by_name_or_null(const std::string& name)
{
  auto cont = allContainers.find(name);
  return cont == allContainers.end() ? nullptr : cont->second;
}

Container* Container::by_name(const std::string& name)
{
  Container* ret = Container::by_name_or_null(name);
  xbt_assert(ret != nullptr, "container with name %s not found", name.c_str());

  return ret;
}

void Container::remove_from_parent()
{
  if (father_) {
    XBT_DEBUG("removeChildContainer (%s) FromContainer (%s) ", get_cname(), father_->get_cname());
    father_->children_.erase(name_);
  }
  delete this;
}

void Container::log_creation()
{
  double timestamp = SIMIX_get_clock();
  std::stringstream stream;

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, PAJE_CreateContainer, timestamp);

  if (trace_format == simgrid::instr::TraceFormat::Paje) {
    stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_CreateContainer << " ";
    stream << timestamp << " " << id_ << " " << type_->get_id() << " " << father_->id_ << " \"";
    if (name_.find("rank-") != 0)
      stream << name_ << "\"";
    else
      /* Subtract -1 because this is the process id and we transform it to the rank id */
      stream << "rank-" << stoi(name_.substr(5)) - 1 << "\"";

    XBT_DEBUG("Dump %s", stream.str().c_str());
    tracing_file << stream.str() << std::endl;
  } else if (trace_format == simgrid::instr::TraceFormat::Ti) {
    // if we are in the mode with only one file
    static std::ofstream* ti_unique_file = nullptr;

    if (tracing_files.empty()) {
      // generate unique run id with time
      prefix = xbt_os_time();
    }

    if (not simgrid::config::get_value<bool>("tracing/smpi/format/ti-one-file") || ti_unique_file == nullptr) {
      std::string folder_name = TRACE_get_filename() + "_files";
      std::string filename    = folder_name + "/" + std::to_string(prefix) + "_" + name_ + ".txt";
#ifdef WIN32
      _mkdir(folder_name.c_str());
#else
      mkdir(folder_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif
      ti_unique_file = new std::ofstream(filename.c_str(), std::ofstream::out);
      xbt_assert(not ti_unique_file->fail(), "Tracefile %s could not be opened for writing", filename.c_str());
      tracing_file << filename << std::endl;
    }
    tracing_files.insert({this, ti_unique_file});
  } else {
    THROW_IMPOSSIBLE;
  }
}

void Container::log_destruction()
{
  std::stringstream stream;
  double timestamp = SIMIX_get_clock();

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, PAJE_DestroyContainer, timestamp);

  if (trace_format == simgrid::instr::TraceFormat::Paje) {
    stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_DestroyContainer << " ";
    stream << timestamp << " " << type_->get_id() << " " << id_;
    XBT_DEBUG("Dump %s", stream.str().c_str());
    tracing_file << stream.str() << std::endl;
  } else if (trace_format == simgrid::instr::TraceFormat::Ti) {
    if (not simgrid::config::get_value<bool>("tracing/smpi/format/ti-one-file") || tracing_files.size() == 1) {
      tracing_files.at(this)->close();
      delete tracing_files.at(this);
    }
    tracing_files.erase(this);
  } else {
    THROW_IMPOSSIBLE;
  }
}

StateType* Container::get_state(const std::string& name)
{
  StateType* ret = static_cast<StateType*>(type_->by_name(name));
  ret->set_calling_container(this);
  return ret;
}

LinkType* Container::get_link(const std::string& name)
{
  LinkType* ret = static_cast<LinkType*>(type_->by_name(name));
  ret->set_calling_container(this);
  return ret;
}

VariableType* Container::get_variable(const std::string& name)
{
  VariableType* ret = static_cast<VariableType*>(type_->by_name(name));
  ret->set_calling_container(this);
  return ret;
}
} // namespace instr
} // namespace simgrid
