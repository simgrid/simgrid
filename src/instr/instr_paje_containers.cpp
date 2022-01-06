/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_containers, instr, "Paje tracing event system (containers)");

namespace simgrid {
namespace instr {

Container* Container::root_container_ = nullptr;              /* the root container */
std::map<std::string, Container*, std::less<>> Container::all_containers_; /* all created containers indexed by name */

NetZoneContainer::NetZoneContainer(const std::string& name, unsigned int level, NetZoneContainer* parent)
    : Container::Container(name, "", parent)
{
  xbt_assert(s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name()), "Element '%s' not found", get_cname());
  if (parent_) {
    std::string type_name = std::string("L") + std::to_string(level);
    type_                 = parent_->type_->by_name_or_create<ContainerType>(type_name);
    parent_->children_.insert({get_name(), this});
    on_creation(*this);
  } else {
    type_         = new ContainerType("0");
    set_root(this);
  }
}

RouterContainer::RouterContainer(const std::string& name, Container* parent)
    : Container::Container(name, "ROUTER", parent)
{
  xbt_assert(parent, "Only the Root container has no parent");
  xbt_assert(s4u::Engine::get_instance()->netpoint_by_name_or_null(get_name()), "Element '%s' not found", get_cname());
}

HostContainer::HostContainer(s4u::Host const& host, NetZoneContainer* parent)
    : Container::Container(host.get_name(), "HOST", parent)
{
  xbt_assert(parent, "Only the Root container has no parent");
  xbt_assert(host.get_netpoint(), "Element '%s' not found", host.get_cname());
}

Container::Container(const std::string& name, const std::string& type_name, Container* parent)
    : name_(name), parent_(parent)
{
  static long long int container_id = 0;
  id_                               = container_id; // id (or alias) of the container
  container_id++;

  if (parent_) {
    XBT_DEBUG("new container %s, child of %s", get_cname(), parent->get_cname());

    if (not type_name.empty()) {
      type_ = parent_->type_->by_name_or_create<ContainerType>(type_name);
      parent_->children_.insert({name_, this});
      on_creation(*this);
    }
  }

  //register all kinds by name
  if (not all_containers_.emplace(name_, this).second)
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("container %s already present in all_containers_", get_cname()));

  XBT_DEBUG("Add container name '%s'", get_cname());
}

Container::~Container()
{
  XBT_DEBUG("destroy container %s", get_cname());
  // Begin with destroying my own children
  for (auto child : children_)
    delete child.second;

  // remove me from the all_containers_ data structure
  all_containers_.erase(name_);

  // obligation to dump previous events because they might reference the container that is about to be destroyed
  last_timestamp_to_dump = simgrid_get_clock();
  dump_buffer(true);

  on_destruction(*this);
}

void Container::create_child(const std::string& name, const std::string& type_name)
{
  new Container(name, type_name, this);
}

Container* Container::by_name_or_null(const std::string& name)
{
  auto cont = all_containers_.find(name);
  return cont == all_containers_.end() ? nullptr : cont->second;
}

Container* Container::by_name(const std::string& name)
{
  Container* ret = Container::by_name_or_null(name);
  xbt_assert(ret != nullptr, "container with name %s not found", name.c_str());

  return ret;
}

void Container::remove_from_parent()
{
  if (parent_) {
    XBT_DEBUG("removeChildContainer (%s) FromContainer (%s) ", get_cname(), parent_->get_cname());
    parent_->children_.erase(name_);
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

EntityValue::EntityValue(const std::string& name, const std::string& color, Type* parent)
    : name_(name), color_(color), parent_(parent)
{
  on_creation(*this);
}

} // namespace instr
} // namespace simgrid
