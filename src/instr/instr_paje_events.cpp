/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/config.hpp>

#include "src/instr/instr_private.hpp"
#include "src/instr/instr_smpi.hpp"
#include "src/smpi/include/private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_events, instr, "Paje tracing event system (events)");
extern FILE* tracing_file;
std::map<container_t, FILE*> tracing_files; // TI specific

namespace simgrid {
namespace instr {

PajeEvent::PajeEvent(Container* container, Type* type, double timestamp, e_event_type eventType)
    : container_(container), type_(type), timestamp_(timestamp), eventType_(eventType)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%.*f", __func__, eventType_, TRACE_precision(), timestamp_);
  if (trace_format == simgrid::instr::TraceFormat::Paje) {
    stream_ << std::fixed << std::setprecision(TRACE_precision());
    stream_ << eventType_ << " " << timestamp_ << " " << type_->get_id() << " " << container_->get_id();
  }
  insertIntoBuffer();
};

StateEvent::StateEvent(Container* container, Type* type, e_event_type event_type, EntityValue* value, TIData* extra)
    : PajeEvent::PajeEvent(container, type, SIMIX_get_clock(), event_type), value(value), extra_(extra)
{
#if HAVE_SMPI
  if (simgrid::config::get_value<bool>("smpi/trace-call-location")) {
    smpi_trace_call_location_t* loc = smpi_trace_get_call_location();
    filename                        = loc->filename;
    linenumber                      = loc->linenumber;
  }
#endif
}

void NewEvent::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;

  stream_ << " " << value->getId();

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  fprintf(tracing_file, "%s\n", stream_.str().c_str());
}

void LinkEvent::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;

  stream_ << " " << value_ << " " << endpoint_->get_id() << " " << key_;

  if (TRACE_display_sizes())
    stream_ << " " << size_;

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  fprintf(tracing_file, "%s\n", stream_.str().c_str());
}

void VariableEvent::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;

  stream_ << " " << value;

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  fprintf(tracing_file, "%s\n", stream_.str().c_str());
}

void StateEvent::print()
{
  if (trace_format == simgrid::instr::TraceFormat::Paje) {

    if (value != nullptr) // PAJE_PopState Event does not need to have a value
      stream_ << " " << value->getId();

    if (TRACE_display_sizes())
      stream_ << " " << ((extra_ != nullptr) ? extra_->display_size() : 0);

#if HAVE_SMPI
    if (simgrid::config::get_value<bool>("smpi/trace-call-location")) {
      stream_ << " \"" << filename << "\" " << linenumber;
    }
#endif
    XBT_DEBUG("Dump %s", stream_.str().c_str());
    fprintf(tracing_file, "%s\n", stream_.str().c_str());
  } else if (trace_format == simgrid::instr::TraceFormat::Ti) {
    if (extra_ == nullptr)
      return;

    /* Unimplemented calls are: WAITANY, SENDRECV, SCAN, EXSCAN, SSEND, and ISSEND. */

    // FIXME: dirty extract "rank-" from the name, as we want the bare process id here
    if (getContainer()->get_name().find("rank-") != 0)
      stream_ << getContainer()->get_name() << " " << extra_->print();
    else
      /* Subtract -1 because this is the process id and we transform it to the rank id */
      stream_ << stoi(getContainer()->get_name().erase(0, 5)) - 1 << " " << extra_->print();

    fprintf(tracing_files.at(getContainer()), "%s\n", stream_.str().c_str());
  } else {
    THROW_IMPOSSIBLE;
  }

  delete extra_;
}
}
}
