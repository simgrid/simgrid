/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_types, instr, "Paje tracing event system (types)");

extern std::ofstream tracing_file;
// to check if variables were previously set to 0, otherwise paje won't simulate them
static std::set<std::string> platform_variables;

namespace simgrid {
namespace instr {

Type::Type(const std::string& name, const std::string& alias, const std::string& color, Type* father)
    : id_(instr_new_paje_id()), name_(name), color_(color), father_(father)
{
  if (name_.empty() || alias.empty())
    throw simgrid::TracingError(XBT_THROW_POINT, "can't create a new type with no name or alias");

  if (father != nullptr){
    father->children_[alias].reset(this);
    XBT_DEBUG("new type %s, child of %s", get_cname(), father->get_cname());
  }
  if (trace_format == simgrid::instr::TraceFormat::Paje) {
    stream_ << std::fixed << std::setprecision(TRACE_precision());
  }
}

ContainerType::ContainerType(const std::string& name, Type* father) : Type(name, name, "", father)
{
  XBT_DEBUG("ContainerType %s(%lld), child of %s(%lld)", get_cname(), get_id(), father->get_cname(), father->get_id());
  log_definition(PAJE_DefineContainerType);
}

EventType::EventType(const std::string& name, Type* father) : ValueType(name, father)
{
  XBT_DEBUG("EventType %s(%lld), child of %s(%lld)", get_cname(), get_id(), father->get_cname(), father->get_id());
  log_definition(PAJE_DefineEventType);
}

StateType::StateType(const std::string& name, Type* father) : ValueType(name, father)
{
  XBT_DEBUG("StateType %s(%lld), child of %s(%lld)", get_cname(), get_id(), father->get_cname(), father->get_id());
  log_definition(PAJE_DefineStateType);
}

void StateType::set_event(const std::string& value_name)
{
  events_.push_back(new StateEvent(issuer_, this, PAJE_SetState, get_entity_value(value_name), nullptr));
}

void StateType::push_event(const std::string& value_name, TIData* extra)
{
  events_.push_back(new StateEvent(issuer_, this, PAJE_PushState, get_entity_value(value_name), extra));
}

void StateType::push_event(const std::string& value_name)
{
  events_.push_back(new StateEvent(issuer_, this, PAJE_PushState, get_entity_value(value_name), nullptr));
}

void StateType::pop_event()
{
  pop_event(nullptr);
}

void StateType::pop_event(TIData* extra)
{
  events_.push_back(new StateEvent(issuer_, this, PAJE_PopState, nullptr, extra));
}

VariableType::VariableType(const std::string& name, const std::string& color, Type* father)
    : Type(name, name, color, father)
{
  XBT_DEBUG("VariableType %s(%lld), child of %s(%lld)", get_cname(), get_id(), father->get_cname(), father->get_id());
  log_definition(PAJE_DefineVariableType);
}

void VariableType::instr_event(double now, double delta, const char* resource, double value)
{
  /* To trace resource utilization, we use AddEvent and SubEvent only. This implies to add a SetEvent first to set the
   * initial value of all variables for subsequent adds/subs. If we don't do so, the first AddEvent would be added to a
   * non-determined value, hence causing analysis problems.
   */

  // create a key considering the resource and variable
  std::string key = std::string(resource) + get_name();

  // check if key exists: if it doesn't, set the variable to zero and mark this in the global map.
  if (platform_variables.find(key) == platform_variables.end()) {
    set_event(now, 0);
    platform_variables.insert(key);
  }

  add_event(now, value);
  sub_event(now + delta, value);
}

void VariableType::set_event(double timestamp, double value)
{
  events_.push_back(new VariableEvent(timestamp, issuer_, this, PAJE_SetVariable, value));
}

void VariableType::add_event(double timestamp, double value)
{
  events_.push_back(new VariableEvent(timestamp, issuer_, this, PAJE_AddVariable, value));
}

void VariableType::sub_event(double timestamp, double value)
{
  events_.push_back(new VariableEvent(timestamp, issuer_, this, PAJE_SubVariable, value));
}

LinkType::LinkType(const std::string& name, const std::string& alias, Type* father) : ValueType(name, alias, father)
{
}
void LinkType::start_event(Container* startContainer, const std::string& value, const std::string& key)
{
  start_event(startContainer, value, key, -1);
}

void LinkType::start_event(Container* startContainer, const std::string& value, const std::string& key, int size)
{
  new LinkEvent(issuer_, this, PAJE_StartLink, startContainer, value, key, size);
}

void LinkType::end_event(Container* endContainer, const std::string& value, const std::string& key)
{
  new LinkEvent(issuer_, this, PAJE_EndLink, endContainer, value, key, -1);
}

void Type::log_definition(e_event_type event_type)
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, event_type, TRACE_precision(), 0.);
  stream_ << event_type << " " << get_id() << " " << father_->get_id() << " " << get_name();
  if (is_colored())
    stream_ << " \"" << color_ << "\"";
  XBT_DEBUG("Dump %s", stream_.str().c_str());
  tracing_file << stream_.str() << std::endl;
}

void Type::log_definition(simgrid::instr::Type* source, simgrid::instr::Type* dest)
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, PAJE_DefineLinkType, TRACE_precision(), 0.);
  stream_ << PAJE_DefineLinkType << " " << get_id() << " " << father_->get_id() << " " << source->get_id();
  stream_ << " " << dest->get_id() << " " << get_name();
  XBT_DEBUG("Dump %s", stream_.str().c_str());
  tracing_file << stream_.str() << std::endl;
}

Type* Type::by_name(const std::string& name)
{
  Type* ret = nullptr;
  for (auto const& elm : children_) {
    if (elm.second->name_ == name) {
      if (ret != nullptr) {
        throw simgrid::TracingError(XBT_THROW_POINT, "there are two children types with the same name?");
      } else {
        ret = elm.second.get();
      }
    }
  }
  if (ret == nullptr)
    throw simgrid::TracingError(
        XBT_THROW_POINT,
        simgrid::xbt::string_printf("type with name (%s) not found in father type (%s)", name.c_str(), get_cname()));
  return ret;
}

void ValueType::add_entity_value(const std::string& name)
{
  add_entity_value(name, "");
}

void ValueType::add_entity_value(const std::string& name, const std::string& color)
{
  if (name.empty())
    throw simgrid::TracingError(XBT_THROW_POINT, "can't get a value with no name");

  auto it = values_.find(name);
  if (it == values_.end()) {
    auto res = values_.emplace(name, EntityValue(name, color, this));
    XBT_DEBUG("new value %s, child of %s", name.c_str(), get_cname());
    res.first->second.print();
  }
}

EntityValue* ValueType::get_entity_value(const std::string& name)
{
  auto ret = values_.find(name);
  if (ret == values_.end()) {
    throw simgrid::TracingError(
        XBT_THROW_POINT,
        simgrid::xbt::string_printf("value with name (%s) not found in father type (%s)", name.c_str(), get_cname()));
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

LinkType* Type::by_name_or_create(const std::string& name, Type* source, Type* dest)
{
  std::string alias = name + "-" + std::to_string(source->id_) + "-" + std::to_string(dest->id_);
  auto it           = children_.find(alias);
  if (it == children_.end()) {
    LinkType* ret = new LinkType(name, alias, this);
    XBT_DEBUG("LinkType %s(%lld), child of %s(%lld)  %s(%lld)->%s(%lld)", ret->get_cname(), ret->get_id(), get_cname(),
              get_id(), source->get_cname(), source->get_id(), dest->get_cname(), dest->get_id());
    ret->log_definition(source, dest);
    return ret;
  } else
    return static_cast<LinkType*>(it->second.get());
}
}
}
