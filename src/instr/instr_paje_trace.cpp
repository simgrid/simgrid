/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "xbt/virtu.h" /* sg_cmdline */
#include <sstream>
#include <vector>
#include <iomanip> /** std::setprecision **/
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr_trace, "tracing event system");

extern FILE * tracing_file;
extern s_instr_trace_writer_t active_writer;

static std::stringstream stream;

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

static void init_stream(PajeEvent* event) {
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int) event->event_type;
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

void DefineContainerEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  print_row();
}

void DefineVariableTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  if (type->color)
    stream << " \"" << type->color << "\"";
  print_row();
}

void DefineStateTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  print_row();
}

void DefineEventTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  print_row();
}

void DefineLinkTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream (this);
  stream << " " << type->id 
         << " " << type->father->id 
         << " " << source->id 
         << " " << dest->id 
         << " " << type->name;
  print_row();
}

void DefineEntityValueEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  stream << " "   << value->id
         << " "   << value->father->id
         << " "   << value->name;
  if(value->color)
    stream << " \"" << value->color << "\"";
  print_row();
}

void CreateContainerEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " "   << container->id
         << " "   << container->type->id
         << " "   << container->father->id
         << " \"" << container->name << "\"";

  print_row();
}

void DestroyContainerEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " "   << container->type->id
         << " "   << container->id;

  print_row();
}

void SetVariableEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value;
  print_row();
}

void AddVariableEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value;
  print_row();
}

void SubVariableEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value;
  print_row();
}

void SetStateEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id;
  stream << " " <<value->id;
#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    stream << " \"" << filename
           << "\" " << linenumber;
  }
#endif
  print_row();
}

void PushStateEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id;
  stream << " " <<value->id;

  if (TRACE_display_sizes()) {
    stream << " ";
    if (extra != nullptr) {
      stream << static_cast<instr_extra_data>(extra)->send_size;
    }
    else {
      stream << 0;
    }
  }
#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    stream << " \"" << filename
           << "\" " << linenumber;
  }
#endif
  print_row();

  if (extra != nullptr) {
    if (static_cast<instr_extra_data>(extra)->sendcounts != nullptr)
      xbt_free(static_cast<instr_extra_data>(extra)->sendcounts);
    if (static_cast<instr_extra_data>(extra)->recvcounts != nullptr)
      xbt_free(static_cast<instr_extra_data>(extra)->recvcounts);
    xbt_free(extra);
  }
}

void PopStateEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id;
  print_row();
}

void ResetStateEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id;
  print_row();
}

void StartLinkEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " <<type->id
         << " " <<container->id
         << " " <<value;
  stream << " " << sourceContainer->id
         << " " << key;

  if (TRACE_display_sizes()) {
    stream << " " << size;
  }
  print_row();
}

void EndLinkEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream(this);
  print_timestamp(this);
  stream << " " <<type->id
         << " " <<container->id
         << " " <<value;
  stream << " " << destContainer->id
         << " " << key;
  print_row();
}

void NewEvent::print () {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  init_stream (this);
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value->id;
  print_row();
}
