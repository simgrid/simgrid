/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_EVENTS_HPP
#define INSTR_PAJE_EVENTS_HPP

#include "src/instr/instr_private.hpp"
#include <string>

namespace simgrid {
namespace instr {
class EntityValue;
class TIData;

enum e_event_type : unsigned int {
  PAJE_DefineContainerType,
  PAJE_DefineVariableType,
  PAJE_DefineStateType,
  PAJE_DefineEventType,
  PAJE_DefineLinkType,
  PAJE_DefineEntityValue,
  PAJE_CreateContainer,
  PAJE_DestroyContainer,
  PAJE_SetVariable,
  PAJE_AddVariable,
  PAJE_SubVariable,
  PAJE_SetState,
  PAJE_PushState,
  PAJE_PopState,
  PAJE_ResetState,
  PAJE_StartLink,
  PAJE_EndLink,
  PAJE_NewEvent
};

class PajeEvent {
  Container* container_;
  Type* type_;

protected:
  Type* getType() { return type_; }
  Container* getContainer() { return container_; }
public:
  double timestamp_;
  e_event_type eventType_;
  PajeEvent(Container* container, Type* type, double timestamp, e_event_type eventType)
      : container_(container), type_(type), timestamp_(timestamp), eventType_(eventType){};
  virtual ~PajeEvent() = default;
  virtual void print() = 0;
  void insertIntoBuffer();
};

class VariableEvent : public PajeEvent {
  double value;

public:
  VariableEvent(double timestamp, Container* container, Type* type, e_event_type event_type, double value);
  void print() override;
};

class StateEvent : public PajeEvent {
  EntityValue* value;
  std::string filename;
  int linenumber = 0;
  TIData* extra_ = nullptr;

public:
  StateEvent(Container* container, Type* type, e_event_type event_type, EntityValue* value);
  StateEvent(Container* container, Type* type, e_event_type event_type, EntityValue* value, TIData* extra);
  void print() override;
};

class LinkEvent : public PajeEvent {
  Container* endpoint_;
  std::string value_;
  std::string key_;
  int size_ = -1;

public:
  LinkEvent(Container* container, Type* type, e_event_type event_type, Container* sourceContainer, std::string value,
            std::string key);
  LinkEvent(Container* container, Type* type, e_event_type event_type, Container* sourceContainer, std::string value,
            std::string key, int size);
  void print() override;
};

class NewEvent : public PajeEvent {
  EntityValue* val;

public:
  NewEvent(double timestamp, Container* container, Type* type, EntityValue* val);
  void print() override;
};
}
}
#endif
