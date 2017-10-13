/* Copyright (c) 2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static simgrid::instr::Type* rootType = nullptr; /* the root type */

namespace simgrid {
namespace instr {

Type::Type(std::string name, std::string alias, std::string color, e_entity_types kind, Type* father)
    : name_(name), color_(color), kind_(kind), father_(father)
{
  if (name.empty() || alias.empty()) {
    THROWF(tracing_error, 0, "can't create a new type with no name or alias");
  }

  id_ = std::to_string(instr_new_paje_id());

  if (father != nullptr){
    father->children_.insert({alias, this});
    XBT_DEBUG("new type %s, child of %s", name_.c_str(), father->getCname());
  }
}

Type::~Type()
{
  for (auto elm : values_)
    delete elm.second;
  for (auto elm : children_)
    delete elm.second;
}

Type* Type::byName(std::string name)
{
  Type* ret = nullptr;
  for (auto elm : children_) {
    if (elm.second->name_ == name) {
      if (ret != nullptr) {
        THROWF (tracing_error, 0, "there are two children types with the same name?");
      } else {
        ret = elm.second;
      }
    }
  }
  if (ret == nullptr)
    THROWF(tracing_error, 2, "type with name (%s) not found in father type (%s)", name.c_str(), getCname());
  return ret;
}

Type* Type::createRootType()
{
  simgrid::instr::Type* ret = new simgrid::instr::Type("0", "0", "", TYPE_CONTAINER, nullptr);
  rootType                  = ret;
  return ret;
}

Type* Type::getRootType()
{
  return rootType;
}

Type* Type::getOrCreateContainerType(std::string name)
{
  auto it = children_.find(name);
  if (it == children_.end()) {
    Type* ret = new simgrid::instr::Type(name, name, "", TYPE_CONTAINER, this);
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
    ret->logContainerTypeDefinition();
    return ret;
  } else
    return it->second;
}

Type* Type::getOrCreateEventType(std::string name)
{
  auto it = children_.find(name);
  if (it == children_.end()) {
    Type* ret = new Type(name, name, "", TYPE_EVENT, this);
    XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
    ret->logDefineEventType();
    return ret;
  } else {
    return it->second;
  }
}

Type* Type::getOrCreateVariableType(std::string name, std::string color)
{
  auto it = children_.find(name);
  if (it == children_.end()) {
    Type* ret = new Type(name, name, color.empty() ? "1 1 1" : color, TYPE_VARIABLE, this);

    XBT_DEBUG("VariableType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
    ret->logVariableTypeDefinition();

    return ret;
  } else
    return it->second;
}

Type* Type::getOrCreateLinkType(std::string name, Type* source, Type* dest)
{
  std::string alias = name + "-" + source->id_ + "-" + dest->id_;
  auto it           = children_.find(alias);
  if (it == children_.end()) {
    Type* ret = new Type(name, alias, "", TYPE_LINK, this);
    XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->getCname(), ret->getId(), getCname(), getId(),
              source->getCname(), source->getId(), dest->getCname(), dest->getId());
    ret->logLinkTypeDefinition(source, dest);
    return ret;
  } else
    return it->second;
}

Type* Type::getOrCreateStateType(std::string name)
{
  auto it = children_.find(name);
  if (it == children_.end()) {
    Type* ret = new Type(name, name, "", TYPE_STATE, this);
    XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
    ret->logStateTypeDefinition();
    return ret;
  } else
    return it->second;
}
}
}
