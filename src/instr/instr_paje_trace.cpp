/* Copyright (c) 2010-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.h"
#include "src/instr/instr_private.hpp"
#include "src/instr/instr_smpi.hpp"
#include "src/smpi/include/private.hpp"
#include "typeinfo"
#include "xbt/virtu.h" /* sg_cmdline */
#include <fstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "tracing event system");

static std::stringstream stream;
FILE *tracing_file = nullptr;

std::map<container_t, FILE*> tracing_files; // TI specific

std::vector<simgrid::instr::PajeEvent*> buffer;
void buffer_debug(std::vector<simgrid::instr::PajeEvent*>* buf);

void dump_comment(std::string comment)
{
  if (comment.empty())
    return;
  fprintf(tracing_file, "# %s\n", comment.c_str());
}

void dump_comment_file(std::string filename)
{
  if (filename.empty())
    return;
  std::ifstream* fs = new std::ifstream();
  fs->open(filename.c_str(), std::ifstream::in);

  if (fs->fail()) {
    THROWF(system_error, 1, "Comment file %s could not be opened for reading.", filename.c_str());
  }
  while (not fs->eof()) {
    std::string line;
    fprintf (tracing_file, "# ");
    std::getline(*fs, line);
    fprintf(tracing_file, "%s", line.c_str());
  }
  fs->close();
}

double TRACE_last_timestamp_to_dump = 0;
//dumps the trace file until the timestamp TRACE_last_timestamp_to_dump
void TRACE_paje_dump_buffer(bool force)
{
  if (not TRACE_is_enabled())
    return;
  XBT_DEBUG("%s: dump until %f. starts", __FUNCTION__, TRACE_last_timestamp_to_dump);
  if (force){
    for (auto const& event : buffer) {
      event->print();
      delete event;
    }
    buffer.clear();
  }else{
    std::vector<simgrid::instr::PajeEvent*>::iterator i = buffer.begin();
    for (auto const& event : buffer) {
      double head_timestamp = event->timestamp_;
      if (head_timestamp > TRACE_last_timestamp_to_dump)
        break;
      event->print();
      delete event;
      ++i;
    }
    buffer.erase(buffer.begin(), i);
  }
  XBT_DEBUG("%s: ends", __FUNCTION__);
}

void buffer_debug(std::vector<simgrid::instr::PajeEvent*>* buf)
{
  return;
  XBT_DEBUG(">>>>>> Dump the state of the buffer. %zu events", buf->size());
  for (auto const& event : *buf) {
    event->print();
    XBT_DEBUG("%p %s", event, stream.str().c_str());
    stream.str("");
    stream.clear();
  }
  XBT_DEBUG("<<<<<<");
}

static void print_row() {
  stream << std::endl;
  fprintf(tracing_file, "%s", stream.str().c_str());
  XBT_DEBUG("Dump %s", stream.str().c_str());
  stream.str("");
  stream.clear();
}

static void print_timestamp(simgrid::instr::PajeEvent* event)
{
  stream << " ";
  /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
  if (event->timestamp_ < 1e-12)
    stream << 0;
  else
    stream << event->timestamp_;
}

/* internal do the instrumentation module */
void simgrid::instr::PajeEvent::insertIntoBuffer()
{
  if (not TRACE_buffer()) {
    print();
    delete this;
    return;
  }
  buffer_debug(&buffer);

  XBT_DEBUG("%s: insert event_type=%d, timestamp=%f, buffersize=%zu)", __FUNCTION__, static_cast<int>(eventType_),
            timestamp_, buffer.size());
  std::vector<simgrid::instr::PajeEvent*>::reverse_iterator i;
  for (i = buffer.rbegin(); i != buffer.rend(); ++i) {
    simgrid::instr::PajeEvent* e1 = *i;
    XBT_DEBUG("compare to %p is of type %d; timestamp:%f", e1, static_cast<int>(e1->eventType_), e1->timestamp_);
    if (e1->timestamp_ <= timestamp_)
      break;
  }
  if (i == buffer.rend())
    XBT_DEBUG("%s: inserted at beginning", __FUNCTION__);
  else if (i == buffer.rbegin())
    XBT_DEBUG("%s: inserted at end", __FUNCTION__);
  else
    XBT_DEBUG("%s: inserted at pos= %zd from its end", __FUNCTION__, std::distance(buffer.rbegin(), i));
  buffer.insert(i.base(), this);

  buffer_debug(&buffer);
}

simgrid::instr::PajeEvent::~PajeEvent()
{
  XBT_DEBUG("%s not implemented for %p: event_type=%d, timestamp=%f", __FUNCTION__, this, (int)eventType_, timestamp_);
}

void TRACE_paje_start() {
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  if (tracing_file == nullptr){
    THROWF (system_error, 1, "Tracefile %s could not be opened for writing.", filename);
  }

  XBT_DEBUG("Filename %s is open for writing", filename);

  /* output generator version */
  fprintf (tracing_file, "#This file was generated using SimGrid-%d.%d.%d\n",
           SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR, SIMGRID_VERSION_PATCH);
  fprintf (tracing_file, "#[");
  unsigned int cpt;
  char *str;
  xbt_dynar_foreach (xbt_cmdline, cpt, str){
    fprintf(tracing_file, "%s ",str);
  }
  fprintf (tracing_file, "]\n");

  /* output one line comment */
  dump_comment (TRACE_get_comment());

  /* output comment file */
  dump_comment_file (TRACE_get_comment_file());

  /* output header */
  TRACE_header(TRACE_basic(),TRACE_display_sizes());
}

void TRACE_paje_end() {
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  XBT_DEBUG("Filename %s is closed", filename);
}

void simgrid::instr::Type::logContainerTypeDefinition()
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, simgrid::instr::PAJE_DefineContainerType);
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, simgrid::instr::PAJE_DefineContainerType,
              TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << simgrid::instr::PAJE_DefineContainerType;
    stream << " " << id_ << " " << father_->getId() << " " << name_;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void simgrid::instr::Type::logVariableTypeDefinition()
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, simgrid::instr::PAJE_DefineVariableType);

  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, simgrid::instr::PAJE_DefineVariableType,
              TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << simgrid::instr::PAJE_DefineVariableType;
    stream << " " << id_ << " " << father_->getId() << " " << name_;
    if (isColored())
      stream << " \"" << color_ << "\"";
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void simgrid::instr::Type::logStateTypeDefinition()
{
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, simgrid::instr::PAJE_DefineStateType,
              TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << simgrid::instr::PAJE_DefineStateType;
    stream << " " << id_ << " " << father_->getId() << " " << name_;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void simgrid::instr::Type::logDefineEventType()
{
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, simgrid::instr::PAJE_DefineEventType,
              TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << simgrid::instr::PAJE_DefineEventType;
    stream << " " << id_ << " " << father_->getId() << " " << name_;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void simgrid::instr::Type::logLinkTypeDefinition(simgrid::instr::Type* source, simgrid::instr::Type* dest)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, simgrid::instr::PAJE_DefineLinkType);
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, simgrid::instr::PAJE_DefineLinkType, TRACE_precision(),
              0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << simgrid::instr::PAJE_DefineLinkType;
    stream << " " << id_ << " " << father_->getId() << " " << source->getId() << " " << dest->getId() << " " << name_;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void simgrid::instr::Value::print()
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, simgrid::instr::PAJE_DefineEntityValue);
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << simgrid::instr::PAJE_DefineEntityValue;
    stream << " " << id_ << " " << father_->getId() << " " << name_;
    if (isColored())
      stream << " \"" << color_ << "\"";
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::SetVariableEvent::SetVariableEvent(double timestamp, container_t container, Type* type, double value)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_SetVariable), value(value)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
}

void simgrid::instr::SetVariableEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_ << " " << value;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::AddVariableEvent::AddVariableEvent(double timestamp, container_t container, simgrid::instr::Type* type,
                                                   double value)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_AddVariable), value(value)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
}

void simgrid::instr::AddVariableEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_ << " " << value;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::SubVariableEvent::SubVariableEvent(double timestamp, container_t container, Type* type, double value)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_SubVariable), value(value)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
}

void simgrid::instr::SubVariableEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_ << " " << value;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::SetStateEvent::SetStateEvent(double timestamp, container_t container, Type* type, Value* value)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_SetState), value(value)
{
#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    smpi_trace_call_location_t* loc = smpi_trace_get_call_location();
    filename   = loc->filename;
    linenumber = loc->linenumber;
  }
#endif

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
}

void simgrid::instr::SetStateEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_;
    stream << " " << value->getId();
#if HAVE_SMPI
    if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
      stream << " \"" << filename << "\" " << linenumber;
    }
#endif
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::PushStateEvent::PushStateEvent(double timestamp, container_t container, Type* type, Value* value,
                                               void* extra)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_PushState), value(value), extra_(extra)
{
#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    smpi_trace_call_location_t* loc = smpi_trace_get_call_location();
    filename   = loc->filename;
    linenumber = loc->linenumber;
  }
#endif

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);

  insertIntoBuffer();
}

simgrid::instr::PushStateEvent::PushStateEvent(double timestamp, container_t container, Type* type, Value* val)
    : PushStateEvent(timestamp, container, type, val, nullptr)
{}

void simgrid::instr::PushStateEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_;
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
    print_row();

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

    char* process_id = nullptr;
    // FIXME: dirty extract "rank-" from the name, as we want the bare process id here
    if (container->getName().find("rank-") != 0)
      process_id = xbt_strdup(container->getCname());
    else
      process_id = xbt_strdup(container->getCname() + 5);

    FILE* trace_file = tracing_files.at(container);

    switch (extra->type) {
      case TRACING_INIT:
        fprintf(trace_file, "%s init\n", process_id);
        break;
      case TRACING_FINALIZE:
        fprintf(trace_file, "%s finalize\n", process_id);
        break;
      case TRACING_SEND:
        fprintf(trace_file, "%s send %d %d %s\n", process_id, extra->dst, extra->send_size, extra->datatype1);
        break;
      case TRACING_ISEND:
        fprintf(trace_file, "%s Isend %d %d %s\n", process_id, extra->dst, extra->send_size, extra->datatype1);
        break;
      case TRACING_RECV:
        fprintf(trace_file, "%s recv %d %d %s\n", process_id, extra->src, extra->send_size, extra->datatype1);
        break;
      case TRACING_IRECV:
        fprintf(trace_file, "%s Irecv %d %d %s\n", process_id, extra->src, extra->send_size, extra->datatype1);
        break;
      case TRACING_TEST:
        fprintf(trace_file, "%s test\n", process_id);
        break;
      case TRACING_WAIT:
        fprintf(trace_file, "%s wait\n", process_id);
        break;
      case TRACING_WAITALL:
        fprintf(trace_file, "%s waitAll\n", process_id);
        break;
      case TRACING_BARRIER:
        fprintf(trace_file, "%s barrier\n", process_id);
        break;
      case TRACING_BCAST: // rank bcast size (root) (datatype)
        fprintf(trace_file, "%s bcast %d ", process_id, extra->send_size);
        if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
          fprintf(trace_file, "%d %s", extra->root, extra->datatype1);
        fprintf(trace_file, "\n");
        break;
      case TRACING_REDUCE: // rank reduce comm_size comp_size (root) (datatype)
        fprintf(trace_file, "%s reduce %d %f ", process_id, extra->send_size, extra->comp_size);
        if (extra->root != 0 || (extra->datatype1 && strcmp(extra->datatype1, "")))
          fprintf(trace_file, "%d %s", extra->root, extra->datatype1);
        fprintf(trace_file, "\n");
        break;
      case TRACING_ALLREDUCE: // rank allreduce comm_size comp_size (datatype)
        fprintf(trace_file, "%s allReduce %d %f %s\n", process_id, extra->send_size, extra->comp_size,
                extra->datatype1);
        break;
      case TRACING_ALLTOALL: // rank alltoall send_size recv_size (sendtype) (recvtype)
        fprintf(trace_file, "%s allToAll %d %d %s %s\n", process_id, extra->send_size, extra->recv_size,
                extra->datatype1, extra->datatype2);
        break;
      case TRACING_ALLTOALLV: // rank alltoallv send_size [sendcounts] recv_size [recvcounts] (sendtype) (recvtype)
        fprintf(trace_file, "%s allToAllV %d ", process_id, extra->send_size);
        for (int i = 0; i < extra->num_processes; i++)
          fprintf(trace_file, "%d ", extra->sendcounts[i]);
        fprintf(trace_file, "%d ", extra->recv_size);
        for (int i = 0; i < extra->num_processes; i++)
          fprintf(trace_file, "%d ", extra->recvcounts[i]);
        fprintf(trace_file, "%s %s \n", extra->datatype1, extra->datatype2);
        break;
      case TRACING_GATHER: // rank gather send_size recv_size root (sendtype) (recvtype)
        fprintf(trace_file, "%s gather %d %d %d %s %s\n", process_id, extra->send_size, extra->recv_size, extra->root,
                extra->datatype1, extra->datatype2);
        break;
      case TRACING_ALLGATHERV: // rank allgatherv send_size [recvcounts] (sendtype) (recvtype)
        fprintf(trace_file, "%s allGatherV %d ", process_id, extra->send_size);
        for (int i = 0; i < extra->num_processes; i++)
          fprintf(trace_file, "%d ", extra->recvcounts[i]);
        fprintf(trace_file, "%s %s \n", extra->datatype1, extra->datatype2);
        break;
      case TRACING_REDUCE_SCATTER: // rank reducescatter [recvcounts] comp_size (sendtype)
        fprintf(trace_file, "%s reduceScatter ", process_id);
        for (int i = 0; i < extra->num_processes; i++)
          fprintf(trace_file, "%d ", extra->recvcounts[i]);
        fprintf(trace_file, "%f %s\n", extra->comp_size, extra->datatype1);
        break;
      case TRACING_COMPUTING:
        fprintf(trace_file, "%s compute %f\n", process_id, extra->comp_size);
        break;
      case TRACING_SLEEPING:
        fprintf(trace_file, "%s sleep %f\n", process_id, extra->sleep_duration);
        break;
      case TRACING_GATHERV: // rank gatherv send_size [recvcounts] root (sendtype) (recvtype)
        fprintf(trace_file, "%s gatherV %d ", process_id, extra->send_size);
        for (int i = 0; i < extra->num_processes; i++)
          fprintf(trace_file, "%d ", extra->recvcounts[i]);
        fprintf(trace_file, "%d %s %s\n", extra->root, extra->datatype1, extra->datatype2);
        break;
      case TRACING_ALLGATHER: // rank allgather sendcount recvcounts (sendtype) (recvtype)
        fprintf(trace_file, "%s allGather %d %d %s %s", process_id, extra->send_size, extra->recv_size,
                extra->datatype1, extra->datatype2);
        break;
      case TRACING_WAITANY:
      case TRACING_SENDRECV:
      case TRACING_SCATTER:
      case TRACING_SCATTERV:
      case TRACING_SCAN:
      case TRACING_EXSCAN:
      case TRACING_COMM_SIZE:
      case TRACING_COMM_SPLIT:
      case TRACING_COMM_DUP:
      case TRACING_SSEND:
      case TRACING_ISSEND:
      default:
        XBT_WARN("Call from %s impossible to translate into replay command : Not implemented (yet)", value->getCname());
        break;
    }

    if (extra->recvcounts != nullptr)
      xbt_free(extra->recvcounts);
    if (extra->sendcounts != nullptr)
      xbt_free(extra->sendcounts);
    xbt_free(process_id);
    xbt_free(extra);

  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::PopStateEvent::PopStateEvent(double timestamp, container_t container, Type* type)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_PopState)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
}

void simgrid::instr::PopStateEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::ResetStateEvent::ResetStateEvent(double timestamp, container_t container, Type* type)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_ResetState)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
  delete[] this;
}

void simgrid::instr::ResetStateEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::StartLinkEvent::StartLinkEvent(double timestamp, container_t container, Type* type,
                                               container_t sourceContainer, std::string value, std::string key)
    : StartLinkEvent(timestamp, container, type, sourceContainer, value, key, -1)
{}

simgrid::instr::StartLinkEvent::StartLinkEvent(double timestamp, container_t container, Type* type,
                                               container_t sourceContainer, std::string value, std::string key,
                                               int size)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_StartLink)
    , sourceContainer_(sourceContainer)
    , value_(value)
    , key_(key)
    , size_(size)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f, value:%s", __FUNCTION__, (int)eventType_, this->timestamp_, this->value_.c_str());
  insertIntoBuffer();
}

void simgrid::instr::StartLinkEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_ << " " << value_;
    stream << " " << sourceContainer_->id_ << " " << key_;

    if (TRACE_display_sizes()) {
      stream << " " << size_;
    }
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::EndLinkEvent::EndLinkEvent(double timestamp, container_t container, Type* type,
                                           container_t destContainer, std::string value, std::string key)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_EndLink)
    , destContainer(destContainer)
    , value(value)
    , key(key)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);
  insertIntoBuffer();
}

void simgrid::instr::EndLinkEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_ << " " << value;
    stream << " " << destContainer->id_ << " " << key;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

simgrid::instr::NewEvent::NewEvent(double timestamp, container_t container, Type* type, Value* val)
    : simgrid::instr::PajeEvent::PajeEvent(container, type, timestamp, PAJE_NewEvent)
{
  this->val                             = val;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)eventType_, this->timestamp_);

  insertIntoBuffer();
}

void simgrid::instr::NewEvent::print()
{
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)eventType_, TRACE_precision(), timestamp_);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->eventType_;
    print_timestamp(this);
    stream << " " << type->getId() << " " << container->id_ << " " << val->getId();
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void TRACE_TI_start()
{
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  if (tracing_file == nullptr)
    THROWF(system_error, 1, "Tracefile %s could not be opened for writing.", filename);

  XBT_DEBUG("Filename %s is open for writing", filename);

  /* output one line comment */
  dump_comment(TRACE_get_comment());

  /* output comment file */
  dump_comment_file(TRACE_get_comment_file());
}

void TRACE_TI_end()
{
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  XBT_DEBUG("Filename %s is closed", filename);
}
