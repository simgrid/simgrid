/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_TYPES_HPP
#define INSTR_PAJE_TYPES_HPP

#include "src/instr/instr_private.hpp"
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace simgrid {
namespace instr {
class ContainerType;
class EventType;

class Type {
  long long int id_;
  std::string name_;
  std::string color_;
  Type* father_;

public:
  std::map<std::string, std::unique_ptr<Type>> children_;
  Container* issuer_ = nullptr;
  std::stringstream stream_;

  Type(const std::string& name, const std::string& alias, const std::string& color, Type* father);
  virtual ~Type() = default;

  const std::string& get_name() const { return name_; }
  const char* get_cname() { return name_.c_str(); }
  long long int get_id() { return id_; }
  bool is_colored() { return not color_.empty(); }

  Type* by_name(const std::string& name);
  LinkType* by_name_or_create(const std::string& name, Type* source, Type* dest);
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

  void log_definition(e_event_type event_type);
  void log_definition(Type* source, Type* dest);
};

class ContainerType : public Type {
public:
  explicit ContainerType(const std::string& name) : Type(name, name, "", nullptr){};
  ContainerType(const std::string& name, Type* father);
};

class VariableType : public Type {
  std::vector<VariableEvent*> events_;
public:
  VariableType(const std::string& name, const std::string& color, Type* father);
  void instr_event(double now, double delta, const char* resource, double value);
  void set_event(double timestamp, double value);
  void add_event(double timestamp, double value);
  void sub_event(double timestamp, double value);
};

class ValueType : public Type {
public:
  std::map<std::string, EntityValue> values_;
  ValueType(const std::string& name, const std::string& alias, Type* father) : Type(name, alias, "", father){};
  ValueType(const std::string& name, Type* father) : Type(name, name, "", father){};
  virtual ~ValueType() = default;
  void add_entity_value(const std::string& name, const std::string& color);
  void add_entity_value(const std::string& name);
  EntityValue* get_entity_value(const std::string& name);
};

class LinkType : public ValueType {
public:
  LinkType(const std::string& name, const std::string& alias, Type* father);
  void start_event(Container* startContainer, const std::string& value, const std::string& key);
  void start_event(Container* startContainer, const std::string& value, const std::string& key, int size);
  void end_event(Container* endContainer, const std::string& value, const std::string& key);
};

class EventType : public ValueType {
public:
  EventType(const std::string& name, Type* father);
};

class StateType : public ValueType {
  std::vector<StateEvent*> events_;
public:
  StateType(const std::string& name, Type* father);
  void set_event(const std::string& value_name);
  void push_event(const std::string& value_name);
  void push_event(const std::string& value_name, TIData* extra);
  void pop_event();
  void pop_event(TIData* extra);
};
}
}
#endif
