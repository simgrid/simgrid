/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"
#include "src/instr/instr_smpi.hpp"
#include "src/smpi/include/private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_events, instr, "Paje tracing event system (events)");
extern std::ofstream tracing_file;
extern std::map<container_t, std::ofstream*> tracing_files; // TI specific

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
  insert_into_buffer();
};

void PajeEvent::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  tracing_file << stream_.str() << std::endl;
}

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

  stream_ << " " << value->get_id();

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  tracing_file << stream_.str() << std::endl;
}

void LinkEvent::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;

  stream_ << " " << value_ << " " << endpoint_->get_id() << " " << key_;

  if (TRACE_display_sizes() && size_ != -1)
    stream_ << " " << size_;

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  tracing_file << stream_.str() << std::endl;
}

void VariableEvent::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;

  stream_ << " " << value_;

  XBT_DEBUG("Dump %s", stream_.str().c_str());
  tracing_file << stream_.str() << std::endl;
}

void StateEvent::print()
{
  if (trace_format == simgrid::instr::TraceFormat::Paje) {

    if (value != nullptr) // PAJE_PopState Event does not need to have a value
      stream_ << " " << value->get_id();

    if (TRACE_display_sizes())
      stream_ << " " << ((extra_ != nullptr) ? extra_->display_size() : "");

#if HAVE_SMPI
    if (simgrid::config::get_value<bool>("smpi/trace-call-location")) {
      stream_ << " \"" << filename << "\" " << linenumber;
    }
#endif
    XBT_DEBUG("Dump %s", stream_.str().c_str());
    tracing_file << stream_.str() << std::endl;
  } else if (trace_format == simgrid::instr::TraceFormat::Ti) {
    if (extra_ == nullptr)
      return;

    /* Unimplemented calls are: WAITANY, SENDRECV, SCAN, EXSCAN, SSEND, and ISSEND. */
    std::string container_name(get_container()->get_name());
    // FIXME: dirty extract "rank-" from the name, as we want the bare process id here
    if (get_container()->get_name().find("rank-") == 0) {
      /* Subtract -1 because this is the process id and we transform it to the rank id */
      container_name=std::to_string(stoi(container_name.erase(0, 5)) - 1);
    }
    #if HAVE_SMPI
    if (simgrid::config::get_value<bool>("smpi/trace-call-location")) {
      stream_ << container_name << " location " << filename << " " << linenumber << std::endl ;
    }
    #endif
    stream_ << container_name << " " << extra_->print();
    *tracing_files.at(get_container()) << stream_.str() << std::endl;
  } else {
    THROW_IMPOSSIBLE;
  }

}
}
}
