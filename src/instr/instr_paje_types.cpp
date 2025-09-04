/* Copyright (c) 2012-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_paje_types.hpp"
#include "simgrid/Exception.hpp"
#include "src/instr/instr_paje_containers.hpp"
#include "src/instr/instr_paje_events.hpp"
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

namespace simgrid::instr {

long long int new_paje_id()
{
  static long long int type_id = 0;
  return type_id++;
}

Type::Type(PajeEventType event_type, const std::string& name, const std::string& alias, const std::string& color,
           Type* parent)
    : name_(name), color_(color), parent_(parent)
{
  if (name_.empty() || alias.empty())
    throw TracingError(XBT_THROW_POINT, "can't create a new type with no name or alias");

  if (parent != nullptr) {
    parent->children_[alias].reset(this);
    XBT_DEBUG("new type %s, child of %s", get_cname(), parent->get_cname());
    on_creation(*this, event_type);
  }
}

void StateType::set_event(const std::string& value_name)
{
  events_.push_back(new StateEvent(get_issuer(), this, PajeEventType::SetState, get_entity_value(value_name), nullptr));
}

void StateType::push_event(const std::string& value_name, TIData* extra)
{
  events_.push_back(new StateEvent(get_issuer(), this, PajeEventType::PushState, get_entity_value(value_name), extra));
}

void StateType::push_event(const std::string& value_name)
{
  events_.push_back(
      new StateEvent(get_issuer(), this, PajeEventType::PushState, get_entity_value(value_name), nullptr));
}

void StateType::pop_event()
{
  pop_event(nullptr);
}

void StateType::pop_event(TIData* extra)
{
  events_.push_back(new StateEvent(get_issuer(), this, PajeEventType::PopState, nullptr, extra));
}

void VariableType::instr_event(double now, double delta, const char* resource, double value)
{
  /* To trace resource utilization, we use AddEvent and SubEvent only. This implies to add a SetEvent first to set the
   * initial value of all variables for subsequent adds/subs. If we don't do so, the first AddEvent would be added to a
   * non-determined value, hence causing analysis problems.
   */

  // to check if variables were previously set to 0, otherwise paje won't simulate them
  // create a key considering the resource and variable, and check if key exists in the global map:
  // if it doesn't, set the variable to zero.
  if (static std::set<std::string, std::less<>> platform_variables;
      platform_variables.emplace(resource + get_name()).second)
    set_event(now, 0);

  add_event(now, value);
  sub_event(now + delta, value);
}

void VariableType::set_event(double timestamp, double value)
{
  events_.push_back(new VariableEvent(timestamp, get_issuer(), this, PajeEventType::SetVariable, value));
}

void VariableType::add_event(double timestamp, double value)
{
  events_.push_back(new VariableEvent(timestamp, get_issuer(), this, PajeEventType::AddVariable, value));
}

void VariableType::sub_event(double timestamp, double value)
{
  events_.push_back(new VariableEvent(timestamp, get_issuer(), this, PajeEventType::SubVariable, value));
}

void LinkType::start_event(Container* startContainer, const std::string& value, const std::string& key, size_t size)
{
  new LinkEvent(get_issuer(), this, PajeEventType::StartLink, startContainer, value, key, size);
}

void LinkType::end_event(Container* endContainer, const std::string& value, const std::string& key)
{
  new LinkEvent(get_issuer(), this, PajeEventType::EndLink, endContainer, value, key);
}

Type* Type::by_name(const std::string& name)
{
  Type* ret = nullptr;
  for (auto const& [_, child] : children_) {
    if (child->name_ == name) {
      if (ret != nullptr) {
        throw TracingError(XBT_THROW_POINT, "there are two children types with the same name?");
      } else {
        ret = child.get();
      }
    }
  }
  if (ret == nullptr)
    throw TracingError(XBT_THROW_POINT, xbt::string_printf("type with name (%s) not found in parent type (%s)",
                                                           name.c_str(), get_cname()));
  return ret;
}

void ValueType::add_entity_value(const std::string& name)
{
  add_entity_value(name, "");
}

void ValueType::add_entity_value(const std::string& name, const std::string& color)
{
  if (name.empty())
    throw TracingError(XBT_THROW_POINT, "can't get a value with no name");

  auto it = values_.find(name);
  if (it == values_.end()) {
    XBT_DEBUG("new value %s, child of %s", name.c_str(), get_cname());
    values_.try_emplace(name, name, color, this);
  }
}

EntityValue* ValueType::get_entity_value(const std::string& name)
{
  auto ret = values_.find(name);
  if (ret == values_.end()) {
    throw TracingError(XBT_THROW_POINT, xbt::string_printf("value with name (%s) not found in parent type (%s)",
                                                           name.c_str(), get_cname()));
  }
  return &ret->second;
}

VariableType* Type::by_name_or_create(const std::string& name, const std::string& color)
{
  auto cont = children_.find(name);
  std::string mycolor = color.empty() ? "1 1 1" : color;
  return cont == children_.end() ? new VariableType(name, mycolor, this)
                                 : static_cast<VariableType*>(cont->second.get());
}

LinkType* Type::by_name_or_create(const std::string& name, const Type* source, const Type* dest)
{
  std::string alias = name + "-" + std::to_string(source->id_) + "-" + std::to_string(dest->id_);
  auto it           = children_.find(alias);
  if (it == children_.end()) {
    return new LinkType(name, source, dest, alias, this);
  } else
    return static_cast<LinkType*>(it->second.get());
}
} // namespace simgrid::instr
