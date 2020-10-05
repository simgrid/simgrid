/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_EVENTS_HPP
#define INSTR_PAJE_EVENTS_HPP

#include "src/instr/instr_private.hpp"
#include "src/internal_config.h"
#include <sstream>
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
public:
  static xbt::signal<void(PajeEvent&)> on_creation;
  static xbt::signal<void(PajeEvent&)> on_destruction;

  double timestamp_;
  e_event_type eventType_;
  std::stringstream stream_;

  PajeEvent(Container* container, Type* type, double timestamp, e_event_type eventType);
  virtual ~PajeEvent();

  Container* get_container() const { return container_; }
  Type* get_type() const { return type_; }

  virtual void print() = 0;
  void insert_into_buffer();
};

class VariableEvent : public PajeEvent {
  double value_;

public:
  VariableEvent(double timestamp, Container* container, Type* type, e_event_type event_type, double value)
      : PajeEvent::PajeEvent(container, type, timestamp, event_type), value_(value)
  {
  }
  void print() override { stream_ << " " << value_; }
};

class StateEvent : public PajeEvent {
  EntityValue* value;
#if HAVE_SMPI
  std::string filename = "(null)";
  int linenumber       = -1;
#endif
  std::unique_ptr<TIData> extra_;

public:
  static xbt::signal<void(StateEvent&)> on_destruction;
  StateEvent(Container* container, Type* type, e_event_type event_type, EntityValue* value, TIData* extra);
  ~StateEvent() override { on_destruction(*this); }
  bool has_extra() const { return extra_ != nullptr; }
  void print() override;
};

class LinkEvent : public PajeEvent {
  Container* endpoint_;
  std::string value_;
  std::string key_;
  int size_ = -1;

public:
  LinkEvent(Container* container, Type* type, e_event_type event_type, Container* sourceContainer,
            const std::string& value, const std::string& key, int size)
      : PajeEvent(container, type, SIMIX_get_clock(), event_type)
      , endpoint_(sourceContainer)
      , value_(value)
      , key_(key)
      , size_(size)
  {
  }
  void print() override;
};

class NewEvent : public PajeEvent {
  EntityValue* value;

public:
  NewEvent(double timestamp, Container* container, Type* type, EntityValue* value)
      : PajeEvent::PajeEvent(container, type, timestamp, PAJE_NewEvent), value(value)
  {
  }
  void print() override;
};
} // namespace instr
} // namespace simgrid
#endif
