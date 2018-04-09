/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

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
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, eventType_, timestamp_);
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
  XBT_DEBUG("%s: event_type=%u, timestamp=%f, value:%s", __func__, eventType_, timestamp_, value_.c_str());
  insertIntoBuffer();
}

VariableEvent::VariableEvent(double timestamp, Container* container, Type* type, e_event_type event_type, double value)
    : PajeEvent::PajeEvent(container, type, timestamp, event_type), value(value)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, eventType_, timestamp_);
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
#else
  filename   = "(null)";
  linenumber = -1;
#endif

  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __func__, eventType_, timestamp_);
  insertIntoBuffer();
};

void NewEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type != instr_fmt_paje)
    return;

  if (timestamp_ < 1e-12)
    stream << eventType_ << " " << 0 << " ";
  else
    stream << eventType_ << " " << timestamp_ << " ";
  stream << getType()->get_id() << " " << getContainer()->get_id() << " " << val->getId();
  XBT_DEBUG("Dump %s", stream.str().c_str());
  fprintf(tracing_file, "%s\n", stream.str().c_str());
}

void LinkEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type != instr_fmt_paje)
    return;
  if (timestamp_ < 1e-12) // FIXME: Why is this hardcoded? What does it stand for? Use a constant variable!
    stream << eventType_ << " " << 0 << " " << getType()->get_id() << " " << getContainer()->get_id();
  else
    stream << eventType_ << " " << timestamp_ << " " << getType()->get_id() << " " << getContainer()->get_id();

  stream << " " << value_ << " " << endpoint_->get_id() << " " << key_;

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
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type != instr_fmt_paje)
    return;

  if (timestamp_ < 1e-12)
    stream << eventType_ << " " << 0 << " ";
  else
    stream << eventType_ << " " << timestamp_ << " ";
  stream << getType()->get_id() << " " << getContainer()->get_id() << " " << value;
  XBT_DEBUG("Dump %s", stream.str().c_str());
  fprintf(tracing_file, "%s\n", stream.str().c_str());
}

void StateEvent::print()
{
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, eventType_, TRACE_precision(), timestamp_);
  if (instr_fmt_type == instr_fmt_paje) {
    if (timestamp_ < 1e-12)
      stream << eventType_ << " " << 0 << " " << getType()->get_id() << " " << getContainer()->get_id();
    else
      stream << eventType_ << " " << timestamp_ << " " << getType()->get_id() << " " << getContainer()->get_id();

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
    if (getContainer()->get_name().find("rank-") != 0)
      stream << getContainer()->get_name() << " " << extra_->print();
    else
      /* Subtract -1 because this is the process id and we transform it to the rank id */
      stream << stoi(getContainer()->get_name().erase(0, 5)) - 1 << " " << extra_->print();

    fprintf(tracing_files.at(getContainer()), "%s\n", stream.str().c_str());
  } else {
    THROW_IMPOSSIBLE;
  }

  delete extra_;
}
}
}
