/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "src/instr/instr_smpi.h"
#include "src/smpi/include/private.hpp"
#include "typeinfo"
#include "xbt/virtu.h" /* sg_cmdline */
#include "simgrid/sg_config.h"

#include <sstream>
#include <vector>
#include <iomanip> /** std::setprecision **/
#include <sys/stat.h>
#ifdef WIN32
#include <direct.h> // _mkdir
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "tracing event system");

static std::stringstream stream;
FILE *tracing_file = nullptr;

static xbt_dict_t tracing_files = nullptr; // TI specific
static double prefix=0.0; // TI specific

static xbt_dynar_t state_tracker = nullptr; //TI specific

std::vector<PajeEvent*> buffer;
void buffer_debug(std::vector<PajeEvent*> *buf);

void dump_comment (const char *comment)
{
  if (not strlen(comment))
    return;
  fprintf (tracing_file, "# %s\n", comment);
}

void dump_comment_file (const char *filename)
{
  if (not strlen(filename))
    return;
  FILE *file = fopen (filename, "r");
  if (not file) {
    THROWF (system_error, 1, "Comment file %s could not be opened for reading.", filename);
  }
  while (not feof(file)) {
    char c;
    c = fgetc(file);
    if (feof(file)) break;
    fprintf (tracing_file, "# ");
    while (c != '\n'){
      fprintf (tracing_file, "%c", c);
      c = fgetc(file);
      if (feof(file)) break;
    }
    fprintf (tracing_file, "\n");
  }
  fclose(file);
}

double TRACE_last_timestamp_to_dump = 0;
//dumps the trace file until the timestamp TRACE_last_timestamp_to_dump
void TRACE_paje_dump_buffer (int force)
{
  if (not TRACE_is_enabled())
    return;
  XBT_DEBUG("%s: dump until %f. starts", __FUNCTION__, TRACE_last_timestamp_to_dump);
  if (force){
    for (auto event : buffer){
      event->print();
      delete event;
    }
    buffer.clear();
  }else{
    std::vector<PajeEvent*>::iterator i = buffer.begin();
    for (auto event :buffer){
      double head_timestamp = event->timestamp;
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

void buffer_debug(std::vector<PajeEvent*> *buf);
void buffer_debug(std::vector<PajeEvent*> *buf) {
  return;
  XBT_DEBUG(">>>>>> Dump the state of the buffer. %zu events", buf->size());
  for (auto event :*buf){
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

static void print_timestamp(PajeEvent* event) {
  stream << " ";
  /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
  if (event->timestamp < 1e-12)
    stream << 0;
  else
    stream << event->timestamp;
}

/* internal do the instrumentation module */
static void insert_into_buffer (PajeEvent* tbi)
{
  if (TRACE_buffer() == 0){
    tbi->print ();
    delete tbi;
    return;
  }
  buffer_debug(&buffer);

  XBT_DEBUG("%s: insert event_type=%d, timestamp=%f, buffersize=%zu)",
      __FUNCTION__, (int)tbi->event_type, tbi->timestamp, buffer.size());
  std::vector<PajeEvent*>::reverse_iterator i;
  for (i = buffer.rbegin(); i != buffer.rend(); ++i) {
    PajeEvent* e1 = *i;
    XBT_DEBUG("compare to %p is of type %d; timestamp:%f", e1,
        (int)e1->event_type, e1->timestamp);
    if (e1->timestamp <= tbi->timestamp)
      break;
  }
  if (i == buffer.rend())
    XBT_DEBUG("%s: inserted at beginning", __FUNCTION__);
  else if (i == buffer.rbegin())
    XBT_DEBUG("%s: inserted at end", __FUNCTION__);
  else
    XBT_DEBUG("%s: inserted at pos= %zd from its end", __FUNCTION__,
        std::distance(buffer.rbegin(),i));
  buffer.insert(i.base(), tbi);

  buffer_debug(&buffer);
}

PajeEvent:: ~PajeEvent()
{
  XBT_DEBUG("%s not implemented for %p: event_type=%d, timestamp=%f", __FUNCTION__,
      this, (int)event_type, timestamp);
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

void DefineContainerEvent(type_t type)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, PAJE_DefineContainerType);
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, PAJE_DefineContainerType, TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DefineContainerType;
    stream << " " << type->id << " " << type->father->id << " " << type->name;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
  //--
}



void LogVariableTypeDefinition(type_t type)
{

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, PAJE_DefineVariableType);

  //print it
if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, PAJE_DefineVariableType, TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DefineVariableType;
    stream << " " << type->id << " " << type->father->id << " " << type->name;
    if (type->color)
      stream << " \"" << type->color << "\"";
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}


void LogStateTypeDefinition(type_t type)
{
  //print it
if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, PAJE_DefineStateType, TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DefineStateType;
    stream << " " << type->id << " " << type->father->id << " " << type->name;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}


void LogDefineEventType(type_t type)
{
  //print it
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, PAJE_DefineEventType, TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DefineEventType;
    stream << " " << type->id << " " << type->father->id << " " << type->name;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void LogLinkTypeDefinition(type_t type, type_t source, type_t dest)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, PAJE_DefineLinkType);
  //print it
if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, PAJE_DefineLinkType, TRACE_precision(), 0.);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DefineLinkType;
    stream << " " << type->id << " " << type->father->id << " " << source->id << " " << dest->id << " " << type->name;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

void LogEntityValue (val_t value)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, PAJE_DefineEntityValue);
  //print it
if (instr_fmt_type == instr_fmt_paje) {
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DefineEntityValue;
    stream << " " << value->id << " " << value->father->id << " " << value->name;
    if (value->color)
      stream << " \"" << value->color << "\"";
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}


void LogContainerCreation (container_t container)
{
  double timestamp                              = SIMIX_get_clock();

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, PAJE_CreateContainer,timestamp);

if (instr_fmt_type == instr_fmt_paje) {
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_CreateContainer;
    stream << " ";
  /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
    if (timestamp < 1e-12)
      stream << 0;
    else
      stream << timestamp;
    stream << " " << container->id << " " << container->type->id << " " << container->father->id << " \""
           << container->name << "\"";

    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    // if we are in the mode with only one file
    static FILE* ti_unique_file = nullptr;

    if (tracing_files == nullptr) {
      tracing_files = xbt_dict_new_homogeneous(nullptr);
      // generate unique run id with time
      prefix = xbt_os_time();
    }

    if (not xbt_cfg_get_boolean("tracing/smpi/format/ti-one-file") || ti_unique_file == nullptr) {
      char* folder_name = bprintf("%s_files", TRACE_get_filename());
      char* filename    = bprintf("%s/%f_%s.txt", folder_name, prefix, container->name);
#ifdef WIN32
      _mkdir(folder_name);
#else
      mkdir(folder_name, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
      ti_unique_file = fopen(filename, "w");
      xbt_assert(ti_unique_file, "Tracefile %s could not be opened for writing: %s", filename, strerror(errno));
      fprintf(tracing_file, "%s\n", filename);

      xbt_free(folder_name);
      xbt_free(filename);
    }

    xbt_dict_set(tracing_files, container->name, (void*)ti_unique_file, nullptr);
  } else {
    THROW_IMPOSSIBLE;
  }
}

void LogContainerDestruction(container_t container)
{
  double timestamp                               = SIMIX_get_clock();

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, PAJE_DestroyContainer, timestamp);

if (instr_fmt_type == instr_fmt_paje) {
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << PAJE_DestroyContainer;
    stream << " ";
  /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
    if (timestamp < 1e-12)
        stream << 0;
    else
      stream << timestamp;
    stream << " " << container->type->id << " " << container->id;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    if (not xbt_cfg_get_boolean("tracing/smpi/format/ti-one-file") || xbt_dict_length(tracing_files) == 1) {
      FILE* f = (FILE*)xbt_dict_get_or_null(tracing_files, container->name);
      fclose(f);
    }
    xbt_dict_remove(tracing_files, container->name);
        } else {
          THROW_IMPOSSIBLE;
        }
}


SetVariableEvent::SetVariableEvent (double timestamp, container_t container, type_t type, double value)
{
  this->event_type                         = PAJE_SetVariable;
  this->timestamp                          = timestamp;
  this->type      = type;
  this->container = container;
  this->value     = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void SetVariableEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id << " " << value;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

AddVariableEvent::AddVariableEvent (double timestamp, container_t container, type_t type, double value)
{
  this->event_type                         = PAJE_AddVariable;
  this->timestamp                          = timestamp;
  this->type      = type;
  this->container = container;
  this->value     = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void AddVariableEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id << " " << value;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

SubVariableEvent::SubVariableEvent (double timestamp, container_t container, type_t type, double value)
{
  this->event_type                         = PAJE_SubVariable;
  this->timestamp                          = timestamp;
  this->type      = type;
  this->container = container;
  this->value     = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void SubVariableEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id << " " << value;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

SetStateEvent::SetStateEvent (double timestamp, container_t container, type_t type, val_t value)
{
  this->event_type                      = PAJE_SetState;
  this->timestamp                       = timestamp;
  this->type      = type;
  this->container = container;
  this->value     = value;

#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    smpi_trace_call_location_t* loc = smpi_trace_get_call_location();
    filename   = loc->filename;
    linenumber = loc->linenumber;
  }
#endif

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void SetStateEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id;
    stream << " " << value->id;
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

PushStateEvent::PushStateEvent (double timestamp, container_t container, type_t type, val_t value, void* extra)
{
  this->event_type                  = PAJE_PushState;
  this->timestamp                   = timestamp;
  this->type = type;
  this->container = container;
  this->value     = value;
  this->extra_     = extra;

#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    smpi_trace_call_location_t* loc = smpi_trace_get_call_location();
    filename   = loc->filename;
    linenumber = loc->linenumber;
  }
#endif

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

PushStateEvent::PushStateEvent (double timestamp, container_t container, type_t type, val_t value)
 : PushStateEvent(timestamp, container, type, value, nullptr)
{}
void PushStateEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id;
    stream << " " << value->id;

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
    if (strstr(container->name, "rank-") == nullptr)
      process_id = xbt_strdup(container->name);
    else
      process_id = xbt_strdup(container->name + 5);

    FILE* trace_file = (FILE*)xbt_dict_get(tracing_files, container->name);

    switch (extra->type) {
      case TRACING_INIT:
        fprintf(trace_file, "%s init\n", process_id);
        break;
      case TRACING_FINALIZE:
        fprintf(trace_file, "%s finalize\n", process_id);
        break;
      case TRACING_SEND:
        fprintf(trace_file, "%s send %d %d %d %s\n", process_id, extra->dst, extra->tag, extra->send_size, extra->datatype1);
        break;
      case TRACING_ISEND:
        fprintf(trace_file, "%s Isend %d %d %d %s\n", process_id, extra->dst, extra->tag, extra->send_size, extra->datatype1);
        break;
      case TRACING_RECV:
        fprintf(trace_file, "%s recv %d %d %d %s\n", process_id, extra->src, extra->tag, extra->send_size, extra->datatype1);
        break;
      case TRACING_IRECV:
        fprintf(trace_file, "%s Irecv %d %d %d %s\n", process_id, extra->src, extra->tag, extra->send_size, extra->datatype1);
        break;
      case TRACING_TEST:
        fprintf(trace_file, "%s test\n", process_id);
        break;
      case TRACING_WAIT:
        fprintf(trace_file, "%s wait %d %d %d\n", process_id, extra->src, extra->dst, extra->tag);
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
      case TRACING_CUSTOM:
	if(extra->print_push)
	  extra->print_push(trace_file, process_id, extra);
	break;
      case TRACING_WAITANY:
      case TRACING_SENDRECV:
      case TRACING_SCATTER:
      case TRACING_SCATTERV:
      case TRACING_ALLGATHER:
      case TRACING_SCAN:
      case TRACING_EXSCAN:
      case TRACING_COMM_SIZE:
      case TRACING_COMM_SPLIT:
      case TRACING_COMM_DUP:
      case TRACING_SSEND:
      case TRACING_ISSEND:
      default:
        XBT_WARN("Call from %s impossible to translate into replay command : Not implemented (yet)", value->name);
        break;
    }
 
    //We need to keep track of the current state, so we can register when the
    //current iteration (TRACING_ITERATION) ends. 
    // We will push the whole 'extra', because we need access to the printing
    // function when the state is popped.
    xbt_dynar_push(state_tracker, &extra);

    xbt_free(process_id);
    //'extra' will be freed in PopStateEvent::print

  } else {
    THROW_IMPOSSIBLE;
  }
}


PopStateEvent::PopStateEvent (double timestamp, container_t container, type_t type)
{
  this->event_type                      = PAJE_PopState;
  this->timestamp                       = timestamp;
  this->type      = type;
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void PopStateEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    char *process_id;

    instr_extra_data extra;
    xbt_dynar_pop(state_tracker, &extra);
    if(extra->type  == TRACING_CUSTOM){
      if (strstr(container->name, "rank-") == nullptr)
	process_id = xbt_strdup(container->name);
      else
	process_id = xbt_strdup(container->name + 5);

      FILE *trace_file = (FILE *)xbt_dict_get(tracing_files, container->name);
      if(extra->print_pop){
        extra->print_pop(trace_file, process_id, extra);
      }
    }
    if (extra->recvcounts != nullptr)
      xbt_free(extra->recvcounts);
    if (extra->sendcounts != nullptr)
      xbt_free(extra->sendcounts);
    xbt_free(extra);
  } else {
    THROW_IMPOSSIBLE;
  }
}

ResetStateEvent::ResetStateEvent (double timestamp, container_t container, type_t type)
{
  this->event_type                        = PAJE_ResetState;
  this->timestamp                         = timestamp;
  this->type      = type;
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
  delete[] this;
}

void ResetStateEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

StartLinkEvent::~StartLinkEvent()
{
  free(value);
  free(key);
}
StartLinkEvent::StartLinkEvent (double timestamp, container_t container,
    type_t type, container_t sourceContainer, const char *value, const char *key)
  : StartLinkEvent(timestamp, container, type, sourceContainer, value, key, -1)
{}

StartLinkEvent::StartLinkEvent (double timestamp, container_t container, type_t type, container_t sourceContainer,
                                const char *value, const char *key, int size)
{
  event_type                             = PAJE_StartLink;
  this->timestamp       = timestamp;
  this->type            = type;
  this->container       = container;
  this->sourceContainer = sourceContainer;
  this->value           = xbt_strdup(value);
  this->key             = xbt_strdup(key);
  this->size            = size;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f, value:%s", __FUNCTION__,
      (int)event_type, this->timestamp, this->value);

  insert_into_buffer (this);
}

void StartLinkEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id << " " << value;
    stream << " " << sourceContainer->id << " " << key;

    if (TRACE_display_sizes()) {
      stream << " " << size;
    }
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

EndLinkEvent::EndLinkEvent (double timestamp, container_t container, type_t type, container_t destContainer,
                      const char *value, const char *key)
{
  this->event_type                         = PAJE_EndLink;
  this->timestamp                          = timestamp;
  this->type          = type;
  this->container     = container;
  this->destContainer = destContainer;
  this->value         = xbt_strdup(value);
  this->key           = xbt_strdup(key);

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

EndLinkEvent::~EndLinkEvent()
{
  free(value);
  free(key);
}
void EndLinkEvent::print() {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id << " " << value;
    stream << " " << destContainer->id << " " << key;
    print_row();
  } else if (instr_fmt_type == instr_fmt_TI) {
    /* Nothing to do */
  } else {
    THROW_IMPOSSIBLE;
  }
}

NewEvent::NewEvent (double timestamp, container_t container, type_t type, val_t value)
{
  this->event_type                      = PAJE_NewEvent;
  this->timestamp                       = timestamp;
  this->type      = type;
  this->container = container;
  this->value     = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void NewEvent::print () {
  if (instr_fmt_type == instr_fmt_paje) {
    XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
    stream << std::fixed << std::setprecision(TRACE_precision());
    stream << (int)this->event_type;
    print_timestamp(this);
    stream << " " << type->id << " " << container->id << " " << value->id;
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
  state_tracker = xbt_dynar_new(sizeof(instr_extra_data), NULL);
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
  xbt_dict_free(&tracing_files);
  xbt_dynar_free(&state_tracker);
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  XBT_DEBUG("Filename %s is closed", filename);
}

