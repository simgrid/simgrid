/* Copyright (c) 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"
#include "src/instr/instr_smpi.hpp"
#include "src/smpi/include/private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_events, instr, "Paje tracing event system (events)");
extern FILE* tracing_file;
std::map<container_t, FILE*> tracing_files; // TI specific

namespace simgrid {
namespace instr {

NewEvent::NewEvent(double timestamp, container_t container, Type* type, EntityValue* val)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_NewEvent), val(val)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, eventType_, timestamp_);
  insertIntoBuffer();
}

LinkEvent::LinkEvent(container_t container, Type* type, e_event_type event_type, container_t endpoint,
                     std::string value, std::string key)
    : LinkEvent(container, type, event_type, endpoint, value, key, -1)
{
}

LinkEvent::LinkEvent(container_t container, Type* type, e_event_type event_type, container_t endpoint,
                     std::string value, std::string key, int size)
    : PajeEvent(container, type, SIMIX_get_clock(), event_type)
    , endpoint_(endpoint)
    , value_(value)
    , key_(key)
    , size_(size)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f, value:%s", __FUNCTION__, eventType_, timestamp_, value_.c_str());
  insertIntoBuffer();
}

VariableEvent::VariableEvent(double timestamp, Container* container, Type* type, e_event_type event_type, double value)
    : PajeEvent::PajeEvent(container, type, timestamp, event_type), value(value)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, eventType_, timestamp_);
  insertIntoBuffer();
}

StateEvent::StateEvent(Container* container, Type* type, e_event_type event_type, EntityValue* value)
    : StateEvent(container, type, event_type, value, nullptr)
{
}

StateEvent::StateEvent(Container* container, Type* type, e_event_type event_type, EntityValue* value, TIData* extra)
    : PajeEvent::PajeEvent(container, type, SIMIX_get_clock(), event_type), value(value), extra_(extra)
{
#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    smpi_trace_call_location_t* loc = smpi_trace_get_call_location();
    filename                        = loc->filename;
    linenumber                      = loc->linenumber;
  }
#endif

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, eventType_, timestamp_);
  insertIntoBuffer();
};

void NewEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __FUNCTION__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type != instr_fmt_paje)
    return;

  if (timestamp_ < 1e-12)
    stream << eventType_ << " " << 0 << " ";
  else
    stream << eventType_ << " " << timestamp_ << " ";
  stream << getType()->getId() << " " << getContainer()->getId() << " " << val->getId();
  XBT_DEBUG("Dump %s", stream.str().c_str());
  fprintf(tracing_file, "%s\n", stream.str().c_str());
}

void LinkEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __FUNCTION__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type != instr_fmt_paje)
    return;
  if (timestamp_ < 1e-12)
    stream << eventType_ << " " << 0 << " " << getType()->getId() << " " << getContainer()->getId();
  else
    stream << eventType_ << " " << timestamp_ << " " << getType()->getId() << " " << getContainer()->getId();

  stream << " " << value_ << " " << endpoint_->getId() << " " << key_;

  if (TRACE_display_sizes()) {
    stream << " " << size_;
  }
  XBT_DEBUG("Dump %s", stream.str().c_str());
  fprintf(tracing_file, "%s\n", stream.str().c_str());
}

void VariableEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __FUNCTION__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type != instr_fmt_paje)
    return;

  if (timestamp_ < 1e-12)
    stream << eventType_ << " " << 0 << " ";
  else
    stream << eventType_ << " " << timestamp_ << " ";
  stream << getType()->getId() << " " << getContainer()->getId() << " " << value;
  XBT_DEBUG("Dump %s", stream.str().c_str());
  fprintf(tracing_file, "%s\n", stream.str().c_str());
}

void StateEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __FUNCTION__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type == instr_fmt_paje) {
    if (timestamp_ < 1e-12)
      stream << eventType_ << " " << 0 << " " << getType()->getId() << " " << getContainer()->getId();
    else
      stream << eventType_ << " " << timestamp_ << " " << getType()->getId() << " " << getContainer()->getId();

    if (value != nullptr) // PAJE_PopState Event does not need to have a value
      stream << " " << value->getId();

    if (TRACE_display_sizes())
      stream << " " << ((extra_ != nullptr) ? extra_->display_size() : 0);

#if HAVE_SMPI
    if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
      stream << " \"" << filename << "\" " << linenumber;
    }
#endif
    XBT_DEBUG("Dump %s", stream.str().c_str());
    fprintf(tracing_file, "%s\n", stream.str().c_str());
  } else if (instr_fmt_type == instr_fmt_TI) {
    if (extra_ == nullptr)
      return;

    /* Unimplemented calls are: WAITANY, SENDRECV, SCAN, EXSCAN, SSEND, and ISSEND. */

    // FIXME: dirty extract "rank-" from the name, as we want the bare process id here
    if (getContainer()->getName().find("rank-") != 0)
      stream << getContainer()->getName() << " " << extra_->print();
    else
      stream << getContainer()->getName().erase(0, 5) << " " << extra_->print();

    fprintf(tracing_files.at(getContainer()), "%s\n", stream.str().c_str());
  } else {
    THROW_IMPOSSIBLE;
  }

  delete extra_;
}
}
}
