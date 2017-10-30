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
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, eventType_, this->timestamp_);
  insertIntoBuffer();
}

LinkEvent::LinkEvent(double timestamp, container_t container, Type* type, e_event_type event_type, container_t endpoint,
                     std::string value, std::string key)
    : LinkEvent(timestamp, container, type, event_type, endpoint, value, key, -1)
{
}

LinkEvent::LinkEvent(double timestamp, container_t container, Type* type, e_event_type event_type, container_t endpoint,
                     std::string value, std::string key, int size)
    : PajeEvent(container, type, timestamp, event_type), endpoint_(endpoint), value_(value), key_(key), size_(size)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f, value:%s", __FUNCTION__, eventType_, timestamp_, value_.c_str());
  insertIntoBuffer();
}

VariableEvent::VariableEvent(double timestamp, Container* container, Type* type, e_event_type event_type, double value)
    : PajeEvent::PajeEvent(container, type, timestamp, event_type), value(value)
{
  XBT_DEBUG("%s: event_type=%u, timestamp=%f", __FUNCTION__, eventType_, this->timestamp_);
  insertIntoBuffer();
}

StateEvent::StateEvent(double timestamp, Container* container, Type* type, e_event_type event_type, EntityValue* value)
    : StateEvent(timestamp, container, type, event_type, value, nullptr)
{
}

StateEvent::StateEvent(double timestamp, Container* container, Type* type, e_event_type event_type, EntityValue* value,
                       void* extra)
    : PajeEvent::PajeEvent(container, type, timestamp, event_type), value(value), extra_(extra)
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
    stream << eventType_ << " " << 0 << " " << type->getId() << " " << container->getId();
  else
    stream << eventType_ << " " << timestamp_ << " " << type->getId() << " " << container->getId();
  stream << " " << val->getId();
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
    stream << eventType_ << " " << 0 << " " << type->getId() << " " << container->getId() << " " << value_;
  else
    stream << eventType_ << " " << timestamp_ << " " << type->getId() << " " << container->getId() << " " << value_;

  stream << " " << endpoint_->getId() << " " << key_;

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
    stream << eventType_ << " " << 0 << " " << type->getId() << " " << container->getId() << " " << value;
  else
    stream << eventType_ << " " << timestamp_ << " " << type->getId() << " " << container->getId() << " " << value;
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
      stream << eventType_ << " " << 0 << " " << type->getId() << " " << container->getId();
    else
      stream << eventType_ << " " << timestamp_ << " " << type->getId() << " " << container->getId();

    if (value != nullptr) // PAJE_PopState Event does not need to have a value
      stream << " " << value->getId();

    if (TRACE_display_sizes()) {
      stream << " ";
      if (extra_ != nullptr) {
        stream << static_cast<instr_extra_data>(extra_)->send_size;
      } else {
        stream << 0;
      }
    }
#if HAVE_SMPI
    if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
      stream << " \"" << filename << "\" " << linenumber;
    }
#endif
    XBT_DEBUG("Dump %s", stream.str().c_str());
    fprintf(tracing_file, "%s\n", stream.str().c_str());

    if (extra_ != nullptr) {
      if (static_cast<instr_extra_data>(extra_)->sendcounts != nullptr)
        xbt_free(static_cast<instr_extra_data>(extra_)->sendcounts);
      if (static_cast<instr_extra_data>(extra_)->recvcounts != nullptr)
        xbt_free(static_cast<instr_extra_data>(extra_)->recvcounts);
      xbt_free(extra_);
    }
  } else if (instr_fmt_type == instr_fmt_TI) {
    if (extra_ == nullptr)
      return;
    instr_extra_data extra = (instr_extra_data)extra_;

    // FIXME: dirty extract "rank-" from the name, as we want the bare process id here
    if (container->getName().find("rank-") != 0)
      stream << container->getName() << " ";
    else
      stream << container->getName().erase(0, 5) << " ";

    FILE* trace_file = tracing_files.at(container);

    switch (extra->type) {
      case TRACING_INIT:
        stream << "init";
        break;
      case TRACING_FINALIZE:
        stream << "finalize";
        break;
      case TRACING_SEND:
        stream << "send " << extra->dst << " " << extra->send_size << " " << extra->datatype1;
        break;
      case TRACING_ISEND:
        stream << "Isend " << extra->dst << " " << extra->send_size << " " << extra->datatype1;
        break;
      case TRACING_RECV:
        stream << "recv " << extra->src << " " << extra->send_size << " " << extra->datatype1;
        break;
      case TRACING_IRECV:
        stream << "Irecv " << extra->src << " " << extra->send_size << " " << extra->datatype1;
        break;
      case TRACING_TEST:
        stream << "test";
        break;
      case TRACING_WAIT:
        stream << "wait";
        break;
      case TRACING_WAITALL:
        stream << "waitAll";
        break;
      case TRACING_BARRIER:
        stream << "barrier";
        break;
      case TRACING_BCAST: // rank bcast size (root) (datatype)
        stream << "bcast " << extra->send_size;
        if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
          stream << " " << extra->root << " " << extra->datatype1;
        break;
      case TRACING_REDUCE: // rank reduce comm_size comp_size (root) (datatype)
        stream << "reduce " << extra->send_size << " " << extra->comp_size;
        if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
          stream << " " << extra->root << " " << extra->datatype1;
        break;
      case TRACING_ALLREDUCE: // rank allreduce comm_size comp_size (datatype)
        stream << "allReduce " << extra->send_size << " " << extra->comp_size << " " << extra->datatype1;
        break;
      case TRACING_ALLTOALL: // rank alltoall send_size recv_size (sendtype) (recvtype)
        stream << "allToAll " << extra->send_size << " " << extra->recv_size << " " << extra->datatype1 << " ";
        stream << extra->datatype2;
        break;
      case TRACING_ALLTOALLV: // rank alltoallv send_size [sendcounts] recv_size [recvcounts] (sendtype) (recvtype)
        stream << "allToAllV " << extra->send_size << " ";
        for (int i = 0; i < extra->num_processes; i++)
          stream << extra->sendcounts[i] << " ";
        stream << extra->recv_size << " ";
        for (int i = 0; i < extra->num_processes; i++)
          stream << extra->recvcounts[i] << " ";
        stream << extra->datatype1 << " " << extra->datatype2;
        break;
      case TRACING_GATHER: // rank gather send_size recv_size root (sendtype) (recvtype)
        stream << "gather " << extra->send_size << " " << extra->recv_size << " " << extra->datatype1 << " ";
        stream << extra->datatype2;
        break;
      case TRACING_ALLGATHERV: // rank allgatherv send_size [recvcounts] (sendtype) (recvtype)
        stream << "allGatherV " << extra->send_size;
        for (int i = 0; i < extra->num_processes; i++)
          stream << extra->recvcounts[i] << " ";
        stream << extra->datatype1 << " " << extra->datatype2;
        break;
      case TRACING_REDUCE_SCATTER: // rank reducescatter [recvcounts] comp_size (sendtype)
        stream << "reduceScatter ";
        for (int i = 0; i < extra->num_processes; i++)
          stream << extra->recvcounts[i] << " ";
        stream << extra->comp_size << " " << extra->datatype1;
        break;
      case TRACING_COMPUTING:
        stream << "compute " << extra->comp_size;
        break;
      case TRACING_SLEEPING:
        stream << "sleep " << extra->sleep_duration;
        break;
      case TRACING_GATHERV: // rank gatherv send_size [recvcounts] root (sendtype) (recvtype)
        stream << "gatherV " << extra->send_size;
        for (int i = 0; i < extra->num_processes; i++)
          stream << extra->recvcounts[i] << " ";
        stream << extra->root << " " << extra->datatype1 << " " << extra->datatype2;
        break;
      case TRACING_ALLGATHER: // rank allgather sendcount recvcounts (sendtype) (recvtype)
        stream << "allGather " << extra->send_size << " " << extra->recv_size << " " << extra->datatype1 << " ";
        stream << extra->datatype2;
        break;
      default:
        /* Unimplemented calls are: WAITANY, SENDRECV, SCATTER, SCATTERV, SCAN, EXSCAN, COMM_SIZE, COMM_SPLIT, COMM_DUP,
         * SSEND, and ISSEND.
         */
        XBT_WARN("Call from %s impossible to translate into replay command : Not implemented (yet)", value->getCname());
        break;
    }
    fprintf(trace_file, "%s\n", stream.str().c_str());

    if (extra->recvcounts != nullptr)
      xbt_free(extra->recvcounts);
    if (extra->sendcounts != nullptr)
      xbt_free(extra->sendcounts);
    xbt_free(extra);

  } else {
    THROW_IMPOSSIBLE;
  }
}
}
}
