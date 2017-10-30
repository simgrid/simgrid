/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_TYPES_HPP
#define INSTR_PAJE_TYPES_HPP

#include "src/instr/instr_private.hpp"
#include <string>
#include <vector>

namespace simgrid {
namespace instr {
enum e_event_type : unsigned int;
class EntityValue;
class ContainerType;
class EventType;
class LinkType;
class StateType;
class VariableType;
class StateEvent;
class VariableEvent;

class Type {
  long long int id_;
  std::string name_;
  std::string color_;
  Type* father_;

public:
  std::map<std::string, Type*> children_;

  Type(std::string name, std::string alias, std::string color, Type* father);
  virtual ~Type();

  std::string getName() { return name_; }
  const char* getCname() { return name_.c_str(); }
  long long int getId() { return id_; }
  bool isColored() { return not color_.empty(); }

  Type* byName(std::string name);
  ContainerType* getOrCreateContainerType(std::string name);
  EventType* getOrCreateEventType(std::string name);
  LinkType* getOrCreateLinkType(std::string name, Type* source, Type* dest);
  StateType* getOrCreateStateType(std::string name);
  VariableType* getOrCreateVariableType(std::string name, std::string color);

  void logDefinition(e_event_type event_type);
  void logDefinition(Type* source, Type* dest);
};

class ContainerType : public Type {
public:
  explicit ContainerType(std::string name) : Type(name, name, "", nullptr){};
  ContainerType(std::string name, Type* father);
};

class VariableType : public Type {
  std::vector<VariableEvent*> events_;
  Container* issuer_ = nullptr;

public:
  VariableType(std::string name, std::string color, Type* father);
  ~VariableType();
  void setCallingContainer(Container* container) { issuer_ = container; }
  void setEvent(double timestamp, double value);
  void addEvent(double timestamp, double value);
  void subEvent(double timestamp, double value);
};

class ValueType : public Type {
public:
  std::map<std::string, EntityValue*> values_;
  ValueType(std::string name, std::string alias, Type* father) : Type(name, alias, "", father){};
  ValueType(std::string name, Type* father) : Type(name, name, "", father){};
  virtual ~ValueType();
  void addEntityValue(std::string name, std::string color);
  void addEntityValue(std::string name);
  EntityValue* getEntityValue(std::string name);
};

class LinkType : public ValueType {
public:
  LinkType(std::string name, std::string alias, Type* father);
  void startEvent(Container* source_container, Container* sourceContainer, std::string value, std::string key);
  void startEvent(Container* source_container, Container* sourceContainer, std::string value, std::string key,
                  int size);
  void endEvent(Container* source_container, Container* destContainer, std::string value, std::string key);
};

class EventType : public ValueType {
public:
  EventType(std::string name, Type* father);
};

class StateType : public ValueType {
  std::vector<StateEvent*> events_;
  Container* issuer_ = nullptr;

public:
  StateType(std::string name, Type* father);
  ~StateType();
  void setCallingContainer(Container* container) { issuer_ = container; }
  void setEvent(std::string value_name);
  void pushEvent(std::string value_name);
  void pushEvent(std::string value_name, void* extra);
  void popEvent();
};
}
}
#endif
