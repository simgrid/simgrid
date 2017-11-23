/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

extern FILE* tracing_file;
extern std::map<container_t, FILE*> tracing_files; // TI specific
double prefix = 0.0;                               // TI specific

static container_t rootContainer = nullptr;    /* the root container */
static std::map<std::string, container_t> allContainers; /* all created containers indexed by name */
std::set<std::string> trivaNodeTypes;           /* all host types defined */
std::set<std::string> trivaEdgeTypes;           /* all link types defined */

long long int instr_new_paje_id ()
{
  static long long int type_id = 0;
  return type_id++;
}

namespace simgrid {
namespace instr {

container_t Container::getRoot()
{
  return rootContainer;
}

NetZoneContainer::NetZoneContainer(std::string name, unsigned int level, NetZoneContainer* father)
    : Container::Container(name, "", father)
{
  netpoint_ = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
  xbt_assert(netpoint_, "Element '%s' not found", name.c_str());
  if (father_) {
    type_ = father_->type_->getOrCreateContainerType(std::string("L") + std::to_string(level));
    father_->children_.insert({getName(), this});
    logCreation();
  } else {
    type_         = new ContainerType("0");
    rootContainer = this;
  }
}

RouterContainer::RouterContainer(std::string name, Container* father) : Container::Container(name, "ROUTER", father)
{
  xbt_assert(father, "Only the Root container has no father");

  netpoint_ = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
  xbt_assert(netpoint_, "Element '%s' not found", name.c_str());

  trivaNodeTypes.insert(type_->getName());
}

HostContainer::HostContainer(simgrid::s4u::Host& host, NetZoneContainer* father)
    : Container::Container(host.getCname(), "HOST", father)
{
  xbt_assert(father, "Only the Root container has no father");

  netpoint_ = host.pimpl_netpoint;
  xbt_assert(netpoint_, "Element '%s' not found", host.getCname());

  trivaNodeTypes.insert(type_->getName());
}

Container::Container(std::string name, std::string type_name, Container* father) : name_(name), father_(father)
{
  static long long int container_id = 0;
  id_                               = container_id; // id (or alias) of the container
  container_id++;

  if (father_) {
    XBT_DEBUG("new container %s, child of %s", name.c_str(), father->name_.c_str());

    if (not type_name.empty()) {
      type_ = father_->type_->getOrCreateContainerType(type_name);
      father_->children_.insert({name_, this});
      logCreation();
    }
  }

  //register all kinds by name
  if (not allContainers.emplace(name_, this).second)
    THROWF(tracing_error, 1, "container %s already present in allContainers data structure", name_.c_str());

  XBT_DEBUG("Add container name '%s'", name_.c_str());

  //register NODE types for triva configuration
  if (type_name == "LINK")
    trivaNodeTypes.insert(type_->getName());
}

Container::~Container()
{
  XBT_DEBUG("destroy container %s", name_.c_str());
  // Begin with destroying my own children
  for (auto child : children_)
    delete child.second;

  // obligation to dump previous events because they might reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = SIMIX_get_clock();
  TRACE_paje_dump_buffer(true);

  // trace my destruction, but not if user requests so or if the container is root
  if (not TRACE_disable_destroy() && this != Container::getRoot())
    logDestruction();

  // remove me from the allContainers data structure
  allContainers.erase(name_);
}

Container* Container::byNameOrNull(std::string name)
{
  auto cont = allContainers.find(name);
  return cont == allContainers.end() ? nullptr : cont->second;
}

Container* Container::byName(std::string name)
{
  Container* ret = Container::byNameOrNull(name);
  if (ret == nullptr)
    THROWF(tracing_error, 1, "container with name %s not found", name.c_str());

  return ret;
}

void Container::removeFromParent()
{
  if (father_) {
    XBT_DEBUG("removeChildContainer (%s) FromContainer (%s) ", getCname(), father_->getCname());
    father_->children_.erase(name_);
  }
}

void Container::logCreation()
{
  double timestamp = SIMIX_get_clock();
  std::stringstream stream;

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, PAJE_CreateContainer, timestamp);

  if (instr_fmt_type == instr_fmt_paje) {
    stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_CreateContainer << " ";
    /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
    if (timestamp < 1e-12)
      stream << 0;
    else
      stream << timestamp;
    stream << " " << id_ << " " << type_->getId() << " " << father_->id_ << " \"" << name_ << "\"";
    XBT_DEBUG("Dump %s", stream.str().c_str());
    fprintf(tracing_file, "%s\n", stream.str().c_str());
  } else if (instr_fmt_type == instr_fmt_TI) {
    // if we are in the mode with only one file
    static FILE* ti_unique_file = nullptr;

    if (tracing_files.empty()) {
      // generate unique run id with time
      prefix = xbt_os_time();
    }

    if (not xbt_cfg_get_boolean("tracing/smpi/format/ti-one-file") || ti_unique_file == nullptr) {
      std::string folder_name = TRACE_get_filename() + "_files";
      std::string filename    = folder_name + "/" + std::to_string(prefix) + "_" + name_ + ".txt";
#ifdef WIN32
      _mkdir(folder_name.c_str());
#else
      mkdir(folder_name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif
      ti_unique_file = fopen(filename.c_str(), "w");
      xbt_assert(ti_unique_file, "Tracefile %s could not be opened for writing: %s", filename.c_str(), strerror(errno));
      fprintf(tracing_file, "%s\n", filename.c_str());
    }
    tracing_files.insert({this, ti_unique_file});
  } else {
    THROW_IMPOSSIBLE;
  }
}

void Container::logDestruction()
{
  std::stringstream stream;
  double timestamp = SIMIX_get_clock();

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, PAJE_DestroyContainer, timestamp);

  if (instr_fmt_type == instr_fmt_paje) {
    stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_DestroyContainer << " ";
    /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
    if (timestamp < 1e-12)
      stream << 0 << " " << type_->getId() << " " << id_;
    else
      stream << timestamp << " " << type_->getId() << " " << id_;
    XBT_DEBUG("Dump %s", stream.str().c_str());
    fprintf(tracing_file, "%s\n", stream.str().c_str());
  } else if (instr_fmt_type == instr_fmt_TI) {
    if (not xbt_cfg_get_boolean("tracing/smpi/format/ti-one-file") || tracing_files.size() == 1) {
      fclose(tracing_files.at(this));
    }
    tracing_files.erase(this);
  } else {
    THROW_IMPOSSIBLE;
  }
}

StateType* Container::getState(std::string name)
{
  StateType* ret = dynamic_cast<StateType*>(type_->byName(name));
  ret->setCallingContainer(this);
  return ret;
}

LinkType* Container::getLink(std::string name)
{
  LinkType* ret = dynamic_cast<LinkType*>(type_->byName(name));
  ret->setCallingContainer(this);
  return ret;
}

VariableType* Container::getVariable(std::string name)
{
  VariableType* ret = dynamic_cast<VariableType*>(type_->byName(name));
  ret->setCallingContainer(this);
  return ret;
}
}
}
