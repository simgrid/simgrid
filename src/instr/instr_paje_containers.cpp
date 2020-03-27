/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

static container_t rootContainer = nullptr;    /* the root container */
static std::map<std::string, container_t> allContainers; /* all created containers indexed by name */

long long int instr_new_paje_id ()
{
  static long long int type_id = 0;
  return type_id++;
}

namespace simgrid {
namespace instr {

Container* Container::get_root()
{
  return rootContainer;
}

NetZoneContainer::NetZoneContainer(const std::string& name, unsigned int level, NetZoneContainer* father)
    : Container::Container(name, "", father)
{
  xbt_assert(s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name()), "Element '%s' not found", get_cname());
  if (father_) {
    std::string type_name = std::string("L") + std::to_string(level);
    type_                 = father_->type_->by_name_or_create<ContainerType>(type_name);
    father_->children_.insert({get_name(), this});
    on_creation(*this);
  } else {
    type_         = new ContainerType("0");
    rootContainer = this;
  }
}

RouterContainer::RouterContainer(const std::string& name, Container* father)
    : Container::Container(name, "ROUTER", father)
{
  xbt_assert(father, "Only the Root container has no father");
  xbt_assert(s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name()), "Element '%s' not found", get_cname());
}

HostContainer::HostContainer(s4u::Host const& host, NetZoneContainer* father)
    : Container::Container(host.get_name(), "HOST", father)
{
  xbt_assert(father, "Only the Root container has no father");
  xbt_assert(host.get_netpoint(), "Element '%s' not found", host.get_cname());
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
      on_creation(*this);
    }
  }

  //register all kinds by name
  if (not allContainers.emplace(name_, this).second)
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("container %s already present in allContainers data structure", get_cname()));

  XBT_DEBUG("Add container name '%s'", get_cname());
}

Container::~Container()
{
  XBT_DEBUG("destroy container %s", get_cname());
  // Begin with destroying my own children
  for (auto child : children_)
    delete child.second;

  // remove me from the allContainers data structure
  allContainers.erase(name_);

  on_destruction(*this);
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

StateType* Container::get_state(const std::string& name)
{
  return static_cast<StateType*>(type_->by_name(name)->set_calling_container(this));
}

LinkType* Container::get_link(const std::string& name)
{
  return static_cast<LinkType*>(type_->by_name(name)->set_calling_container(this));
}

VariableType* Container::get_variable(const std::string& name)
{
  return static_cast<VariableType*>(type_->by_name(name)->set_calling_container(this));
}
} // namespace instr
} // namespace simgrid
