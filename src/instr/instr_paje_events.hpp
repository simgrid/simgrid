/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PAJE_EVENTS_HPP
#define INSTR_PAJE_EVENTS_HPP

#include "src/instr/instr_private.hpp"
#include "src/internal_config.h"
#include <memory>
#include <sstream>
#include <string>

namespace simgrid {
namespace instr {
class EntityValue;
class TIData;

enum class PajeEventType : unsigned int {
  DefineContainerType,
  DefineVariableType,
  DefineStateType,
  DefineEventType,
  DefineLinkType,
  DefineEntityValue,
  CreateContainer,
  DestroyContainer,
  SetVariable,
  AddVariable,
  SubVariable,
  SetState,
  PushState,
  PopState,
  ResetState,
  StartLink,
  EndLink,
  NewEvent
};

inline std::ostream& operator<<(std::ostream& os, PajeEventType event)
{
  return os << static_cast<std::underlying_type_t<PajeEventType>>(event);
}

class PajeEvent {
  Container* container_;
  Type* type_;
  static xbt::signal<void(PajeEvent&)> on_creation;
  static xbt::signal<void(PajeEvent const&)> on_destruction;

public:
  static void on_creation_cb(const std::function<void(PajeEvent&)>& cb) { on_creation.connect(cb); }
  static void on_destruction_cb(const std::function<void(PajeEvent const&)>& cb) { on_destruction.connect(cb); }

  double timestamp_;
  PajeEventType eventType_;
  std::stringstream stream_;

  PajeEvent(Container* container, Type* type, double timestamp, PajeEventType eventType);
  virtual ~PajeEvent();

  Container* get_container() const { return container_; }
  Type* get_type() const { return type_; }

  virtual void print() = 0;
  void insert_into_buffer();
};

class VariableEvent : public PajeEvent {
  double value_;

public:
  VariableEvent(double timestamp, Container* container, Type* type, PajeEventType event_type, double value)
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

  static xbt::signal<void(StateEvent const&)> on_destruction;

public:
  static void on_destruction_cb(const std::function<void(StateEvent const&)>& cb) { on_destruction.connect(cb); }
  StateEvent(Container* container, Type* type, PajeEventType event_type, EntityValue* value, TIData* extra);
  ~StateEvent() override { on_destruction(*this); }
  bool has_extra() const { return extra_ != nullptr; }
  void print() override;
};

class LinkEvent : public PajeEvent {
  Container* endpoint_;
  std::string value_;
  std::string key_;
  size_t size_;

public:
  LinkEvent(Container* container, Type* type, PajeEventType event_type, Container* sourceContainer,
            const std::string& value, const std::string& key, size_t size = static_cast<size_t>(-1))
      : PajeEvent(container, type, simgrid_get_clock(), event_type)
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
      : PajeEvent::PajeEvent(container, type, timestamp, PajeEventType::NewEvent), value(value)
  {
  }
  void print() override;
};
} // namespace instr
} // namespace simgrid
#endif
