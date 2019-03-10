/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_TYPES_HPP
#define INSTR_PAJE_TYPES_HPP

#include "src/instr/instr_private.hpp"
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
  std::map<std::string, Type*> children_;
  Container* issuer_ = nullptr;
  std::stringstream stream_;

  Type(std::string name, std::string alias, std::string color, Type* father);
  virtual ~Type();

  const std::string& get_name() const { return name_; }
  const char* get_cname() { return name_.c_str(); }
  long long int get_id() { return id_; }
  bool is_colored() { return not color_.empty(); }

  Type* by_name(const std::string& name);
  LinkType* by_name_or_create(const std::string& name, Type* source, Type* dest);
  VariableType* by_name_or_create(const std::string& name, std::string color);

  template <class T> T* by_name_or_create(const std::string& name)
  {
    auto cont = children_.find(name);
    return cont == children_.end() ? new T(name, this) : static_cast<T*>(cont->second);
  }

  void set_calling_container(Container* container) { issuer_ = container; }

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
  VariableType(const std::string& name, std::string color, Type* father);
  ~VariableType();
  void instr_event(double now, double delta, const char* resource, double value);
  void set_event(double timestamp, double value);
  void add_event(double timestamp, double value);
  void sub_event(double timestamp, double value);
};

class ValueType : public Type {
public:
  std::map<std::string, EntityValue*> values_;
  ValueType(std::string name, std::string alias, Type* father) : Type(std::move(name), std::move(alias), "", father){};
  ValueType(const std::string& name, Type* father) : Type(name, name, "", father){};
  virtual ~ValueType();
  void add_entity_value(const std::string& name, std::string color);
  void add_entity_value(const std::string& name);
  EntityValue* get_entity_value(const std::string& name);
};

class LinkType : public ValueType {
public:
  LinkType(std::string name, std::string alias, Type* father);
  void start_event(Container* startContainer, std::string value, std::string key);
  void start_event(Container* startContainer, std::string value, std::string key, int size);
  void end_event(Container* endContainer, std::string value, std::string key);
};

class EventType : public ValueType {
public:
  EventType(std::string name, Type* father);
};

class StateType : public ValueType {
  std::vector<StateEvent*> events_;
public:
  StateType(std::string name, Type* father);
  ~StateType();
  void set_event(const std::string& value_name);
  void push_event(const std::string& value_name);
  void push_event(const std::string& value_name, TIData* extra);
  void pop_event();
  void pop_event(TIData* extra);
};
}
}
#endif
