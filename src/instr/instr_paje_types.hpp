/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_TYPES_HPP
#define INSTR_PAJE_TYPES_HPP

#include "src/instr/instr_private.hpp"
#include <memory>
#include <string>
#include <vector>

namespace simgrid {
namespace instr {
class ContainerType;
class EventType;

long long int new_paje_id();

class Type {
  long long int id_ = new_paje_id();
  std::string name_;
  std::string color_;
  Type* parent_;
  std::map<std::string, std::unique_ptr<Type>, std::less<>> children_;
  Container* issuer_ = nullptr;

protected:
  Container* get_issuer() const { return issuer_; }

public:
  static xbt::signal<void(Type const&, PajeEventType event_type)> on_creation;

  Type(PajeEventType event_type, const std::string& name, const std::string& alias, const std::string& color,
       Type* parent);
  virtual ~Type() = default;

  long long int get_id() const { return id_; }
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  const std::string& get_color() const { return color_; }
  Type* get_parent() const { return parent_; }
  const std::map<std::string, std::unique_ptr<Type>, std::less<>>& get_children() const { return children_; }
  bool is_colored() const { return not color_.empty(); }

  Type* by_name(const std::string& name);
  LinkType* by_name_or_create(const std::string& name, const Type* source, const Type* dest);
  VariableType* by_name_or_create(const std::string& name, const std::string& color);

  template <class T> T* by_name_or_create(const std::string& name)
  {
    auto cont = children_.find(name);
    return cont == children_.end() ? new T(name, this) : static_cast<T*>(cont->second.get());
  }

  Type* set_calling_container(Container* container)
  {
    issuer_ = container;
    return this;
  }
};

class ContainerType : public Type {
public:
  explicit ContainerType(const std::string& name) : Type(PajeEventType::DefineContainerType, name, name, "", nullptr){};
  ContainerType(const std::string& name, Type* parent)
      : Type(PajeEventType::DefineContainerType, name, name, "", parent){};
};

class VariableType : public Type {
  std::vector<VariableEvent*> events_;
public:
  VariableType(const std::string& name, const std::string& color, Type* parent)
      : Type(PajeEventType::DefineVariableType, name, name, color, parent)
  {
  }
  void instr_event(double now, double delta, const char* resource, double value);
  void set_event(double timestamp, double value);
  void add_event(double timestamp, double value);
  void sub_event(double timestamp, double value);
};

class ValueType : public Type {
public:
  std::map<std::string, EntityValue, std::less<>> values_;
  ValueType(PajeEventType event_type, const std::string& name, const std::string& alias, Type* parent)
      : Type(event_type, name, alias, "", parent){};
  ValueType(PajeEventType event_type, const std::string& name, Type* parent)
      : Type(event_type, name, name, "", parent){};
  void add_entity_value(const std::string& name, const std::string& color);
  void add_entity_value(const std::string& name);
  EntityValue* get_entity_value(const std::string& name);
};

class LinkType : public ValueType {
public:
  static xbt::signal<void(LinkType const&, Type const&, Type const&)> on_creation;
  LinkType(const std::string& name, const Type* source, const Type* dest, const std::string& alias, Type* parent)
      : ValueType(PajeEventType::DefineLinkType, name, alias, parent)
  {
    on_creation(*this, *source, *dest);
  }
  void start_event(Container* startContainer, const std::string& value, const std::string& key,
                   size_t size = static_cast<size_t>(-1));
  void end_event(Container* endContainer, const std::string& value, const std::string& key);
};

class EventType : public ValueType {
public:
  EventType(const std::string& name, Type* parent) : ValueType(PajeEventType::DefineEventType, name, parent) {}
};

class StateType : public ValueType {
  std::vector<StateEvent*> events_;
public:
  StateType(const std::string& name, Type* parent) : ValueType(PajeEventType::DefineStateType, name, parent) {}
  void set_event(const std::string& value_name);
  void push_event(const std::string& value_name);
  void push_event(const std::string& value_name, TIData* extra);
  void pop_event();
  void pop_event(TIData* extra);
};
} // namespace instr
} // namespace simgrid
#endif
