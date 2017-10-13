/* Copyright (c) 2012, 2014-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

static simgrid::instr::Type* rootType = nullptr; /* the root type */

simgrid::instr::Type* PJ_type_get_root()
{
  return rootType;
}

namespace simgrid {
namespace instr {

Type::Type(std::string name, std::string alias, std::string color, e_entity_types kind, Type* father)
    : name_(name), color_(color), kind_(kind), father_(father)
{
  if (name.empty() || alias.empty()) {
    THROWF(tracing_error, 0, "can't create a new type with name or key equal nullptr");
  }

  id_ = std::to_string(instr_new_paje_id());

  if (father != nullptr){
    father->children_.insert({alias, this});
    XBT_DEBUG("new type %s, child of %s", name_.c_str(), father->getCname());
  }
}

Type::~Type()
{
  for (auto elm : values_) {
    XBT_DEBUG("free value %s, child of %s", elm.second->getCname(), elm.second->father_->getCname());
    delete elm.second;
  }
  for (auto elm : children_) {
    delete elm.second;
  }
}

Type* Type::byName(std::string name)
{
  Type* ret = this->getChildOrNull(name);
  if (ret == nullptr)
    THROWF(tracing_error, 2, "type with name (%s) not found in father type (%s)", name.c_str(), getCname());
  return ret;
}

Type* Type::getChildOrNull(std::string name)
{
  xbt_assert(not name.empty(), "can't get type with a nullptr name");

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
  return ret;
}

Type* Type::containerNew(const char* name, Type* father)
{
  if (name == nullptr){
    THROWF (tracing_error, 0, "can't create a container type with a nullptr name");
  }

  simgrid::instr::Type* ret = new simgrid::instr::Type(name, name, "", TYPE_CONTAINER, father);
  if (father == nullptr) {
    rootType = ret;
  } else {
    XBT_DEBUG("ContainerType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), father->getCname(),
              father->getId());
    ret->logContainerTypeDefinition();
  }
  return ret;
}

Type* Type::addEventType(std::string name)
{
  if (name.empty()) {
    THROWF(tracing_error, 0, "can't create an event type with no name");
  }

  Type* ret = new Type(name, name, "", TYPE_EVENT, this);
  XBT_DEBUG("EventType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
  ret->logDefineEventType();
  return ret;
}

Type* Type::addVariableType(std::string name, std::string color)
{
  if (name.empty())
    THROWF(tracing_error, 0, "can't create a variable type with no name");

  Type* ret = new Type(name, name, color.empty() ? "1 1 1" : color, TYPE_VARIABLE, this);

  XBT_DEBUG("VariableType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
  ret->logVariableTypeDefinition();

  return ret;
}

Type* Type::addLinkType(std::string name, Type* source, Type* dest)
{
  if (name.empty()) {
    THROWF(tracing_error, 0, "can't create a link type with no name");
  }

  std::string alias = name + "-" + source->id_ + "-" + dest->id_;
  Type* ret         = new Type(name, alias, "", TYPE_LINK, this);
  XBT_DEBUG("LinkType %s(%s), child of %s(%s)  %s(%s)->%s(%s)", ret->getCname(), ret->getId(), getCname(), getId(),
            source->getCname(), source->getId(), dest->getCname(), dest->getId());
  ret->logLinkTypeDefinition(source, dest);
  return ret;
}

Type* Type::addStateType(std::string name)
{
  Type* ret = new Type(name, name, "", TYPE_STATE, this);
  XBT_DEBUG("StateType %s(%s), child of %s(%s)", ret->getCname(), ret->getId(), getCname(), getId());
  ret->logStateTypeDefinition();
  return ret;
}
}
}
