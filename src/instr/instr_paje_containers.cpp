/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "surf/surf.h"
#include "src/instr/instr_private.hpp"

#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

static container_t rootContainer = nullptr;    /* the root container */
static std::unordered_map<std::string, container_t> allContainers; /* all created containers indexed by name */
std::set<std::string> trivaNodeTypes;           /* all host types defined */
std::set<std::string> trivaEdgeTypes;           /* all link types defined */

long long int instr_new_paje_id ()
{
  static long long int type_id = 0;
  return type_id++;
}

container_t PJ_container_get_root()
{
  return rootContainer;
}

void PJ_container_set_root (container_t root)
{
  rootContainer = root;
}

namespace simgrid {
namespace instr {

Container::Container(std::string name, e_container_types kind, Container* father)
    : name_(name), kind_(kind), father_(father)
{
  static long long int container_id = 0;
  id_                               = std::to_string(container_id); // id (or alias) of the container
  container_id++;

  //Search for network_element_t
  switch (kind){
    case INSTR_HOST:
      this->netpoint_ = sg_host_by_name(name.c_str())->pimpl_netpoint;
      xbt_assert(this->netpoint_, "Element '%s' not found", name.c_str());
      break;
    case INSTR_ROUTER:
      this->netpoint_ = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
      xbt_assert(this->netpoint_, "Element '%s' not found", name.c_str());
      break;
    case INSTR_AS:
      this->netpoint_ = simgrid::s4u::Engine::getInstance()->getNetpointByNameOrNull(name);
      xbt_assert(this->netpoint_, "Element '%s' not found", name.c_str());
      break;
    default:
      this->netpoint_ = nullptr;
      break;
  }

  if (father_) {
    this->level_ = father_->level_ + 1;
    XBT_DEBUG("new container %s, child of %s", name.c_str(), father->name_.c_str());
  }

  // type definition (method depends on kind of this new container)
  if (this->kind_ == INSTR_AS) {
    //if this container is of an AS, its type name depends on its level
    char as_typename[INSTR_DEFAULT_STR_SIZE];
    snprintf(as_typename, INSTR_DEFAULT_STR_SIZE, "L%d", this->level_);
    if (this->father_) {
      this->type_ = this->father_->type_->getChildOrNull(as_typename);
      if (this->type_ == nullptr) {
        this->type_ = Type::containerNew(as_typename, this->father_->type_);
      }
    }else{
      this->type_ = Type::containerNew("0", nullptr);
    }
  } else {
    //otherwise, the name is its kind
    char typeNameBuff[INSTR_DEFAULT_STR_SIZE];
    switch (this->kind_) {
      case INSTR_HOST:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "HOST");
        break;
      case INSTR_LINK:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "LINK");
        break;
      case INSTR_ROUTER:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "ROUTER");
        break;
      case INSTR_SMPI:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MPI");
        break;
      case INSTR_MSG_PROCESS:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MSG_PROCESS");
        break;
      case INSTR_MSG_VM:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MSG_VM");
        break;
      case INSTR_MSG_TASK:
        snprintf (typeNameBuff, INSTR_DEFAULT_STR_SIZE, "MSG_TASK");
        break;
      default:
        THROWF (tracing_error, 0, "new container kind is unknown.");
        break;
    }
    Type* type = this->father_->type_->getChildOrNull(typeNameBuff);
    if (type == nullptr){
      this->type_ = Type::containerNew(typeNameBuff, this->father_->type_);
    }else{
      this->type_ = type;
    }
  }
  if (this->father_) {
    this->father_->children_.insert({this->name_, this});
    LogContainerCreation(this);
  }

  //register all kinds by name
  if (not allContainers.emplace(this->name_, this).second) {
    THROWF(tracing_error, 1, "container %s already present in allContainers data structure", this->name_.c_str());
  }

  XBT_DEBUG("Add container name '%s'", this->name_.c_str());

  //register NODE types for triva configuration
  if (this->kind_ == INSTR_HOST || this->kind_ == INSTR_LINK || this->kind_ == INSTR_ROUTER) {
    trivaNodeTypes.insert(this->type_->getName());
  }
}

Container::~Container()
{
  XBT_DEBUG("destroy container %s", name_.c_str());
  // Begin with destroying my own children
  for (auto child : children_) {
    delete child.second;
  }

  // obligation to dump previous events because they might reference the container that is about to be destroyed
  TRACE_last_timestamp_to_dump = surf_get_clock();
  TRACE_paje_dump_buffer(1);

  // trace my destruction
  if (not TRACE_disable_destroy() && this != PJ_container_get_root()) {
    // do not trace the container destruction if user requests or if the container is root
    LogContainerDestruction(this);
  }

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
    XBT_DEBUG("removeChildContainer (%s) FromContainer (%s) ", name_.c_str(), father_->name_.c_str());
    father_->children_.erase(name_);
  }
}
}
}
