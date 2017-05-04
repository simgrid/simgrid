/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "src/instr/instr_smpi.h"
#include "src/smpi/private.hpp"
#include "typeinfo"
#include "xbt/virtu.h" /* sg_cmdline */
#include <sstream>
#include <vector>
#include <iomanip> /** std::setprecision **/
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr, "tracing event system");

extern FILE * tracing_file;
extern s_instr_trace_writer_t active_writer;

static std::stringstream stream;
FILE *tracing_file = nullptr;

void print_NULL(PajeEvent* event){}

/* The active set of functions for the selected trace format
 * By default, they all do nothing, hence the print_NULL to avoid segfaults */

s_instr_trace_writer_t active_writer = {&print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL,
                                        &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL,
                                        &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL};

std::vector<PajeEvent*> buffer;
void buffer_debug(std::vector<PajeEvent*> *buf);

void dump_comment (const char *comment)
{
  if (!strlen(comment)) return;
  fprintf (tracing_file, "# %s\n", comment);
}

void dump_comment_file (const char *filename)
{
  if (!strlen(filename)) return;
  FILE *file = fopen (filename, "r");
  if (!file){
    THROWF (system_error, 1, "Comment file %s could not be opened for reading.", filename);
  }
  while (!feof(file)){
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
  if (!TRACE_is_enabled()) return;
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
  buffer.insert(i.base(), tbi);
  if (i == buffer.rend())
    XBT_DEBUG("%s: inserted at beginning", __FUNCTION__);
  else if (i == buffer.rbegin())
    XBT_DEBUG("%s: inserted at end", __FUNCTION__);
  else
    XBT_DEBUG("%s: inserted at pos= %zd from its end", __FUNCTION__,
        std::distance(buffer.rbegin(),i));

  buffer_debug(&buffer);
}

PajeEvent:: ~PajeEvent()
{
  XBT_DEBUG("%s not implemented for %p: event_type=%d, timestamp=%f", __FUNCTION__,
      this, (int)event_type, timestamp);
//  xbt_backtrace_display_current();

 /* switch (event->event_type){
  case PAJE_StartLink:
    xbt_free (((startLink_t)(event->data))->value);
    xbt_free (((startLink_t)(event->data))->key);
    break;
  case PAJE_EndLink:
    xbt_free (((endLink_t)(event->data))->value);
    xbt_free (((endLink_t)(event->data))->key);
    break;
  default:
    break;
  }*/
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

DefineContainerEvent::DefineContainerEvent(type_t type)
{

  event_type                            = PAJE_DefineContainerType;
  timestamp                             = 0;
  this->type = type;
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);
  //print it
  print ();
}

void DefineContainerEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  print_row();
}



DefineVariableTypeEvent::DefineVariableTypeEvent(type_t type)
{
  this->event_type                           = PAJE_DefineVariableType;
  this->timestamp                            = 0;
  this->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);

  //print it
  print ();
}

void DefineVariableTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  if (type->color)
    stream << " \"" << type->color << "\"";
  print_row();
}

DefineStateTypeEvent::DefineStateTypeEvent(type_t type)
{
  this->event_type                        = PAJE_DefineStateType;
  this->timestamp                         = 0;
  this->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);

  //print it
  print();
}


DefineEventTypeEvent::DefineEventTypeEvent(type_t type)
{
  this->event_type                        = PAJE_DefineEventType;
  this->timestamp                         = 0;
  this->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);

  //print it
  print();
}


void DefineStateTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  print_row();
}

void DefineEventTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  stream << " " << type->id
         << " " << type->father->id
         << " " << type->name;
  print_row();
}

DefineLinkTypeEvent::DefineLinkTypeEvent(type_t type, type_t source, type_t dest)
{
  this->event_type                         = PAJE_DefineLinkType;
  this->timestamp                          = 0;
  this->type   = type;
  this->source = source;
  this->dest   = dest;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);

  //print it
  print();
}

void DefineLinkTypeEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  stream << " " << type->id 
         << " " << type->father->id 
         << " " << source->id 
         << " " << dest->id 
         << " " << type->name;
  print_row();
}

DefineEntityValueEvent::DefineEntityValueEvent (val_t value)
{
  this->event_type                           = PAJE_DefineEntityValue;
  this->timestamp                            = 0;
  this->value = value;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);

  //print it
  print();
}


void DefineEntityValueEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  stream << " "   << value->id
         << " "   << value->father->id
         << " "   << value->name;
  if(value->color)
    stream << " \"" << value->color << "\"";
  print_row();
}

CreateContainerEvent::CreateContainerEvent (container_t container)
{
  this->event_type                             = PAJE_CreateContainer;
  this->timestamp                              = SIMIX_get_clock();
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  //print it
  print();
}

void CreateContainerEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " "   << container->id
         << " "   << container->type->id
         << " "   << container->father->id
         << " \"" << container->name << "\"";

  print_row();
}

DestroyContainerEvent::DestroyContainerEvent (container_t container)
{
  this->event_type                              = PAJE_DestroyContainer;
  this->timestamp                               = SIMIX_get_clock();
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  print();
}

void DestroyContainerEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " "   << container->type->id
         << " "   << container->id;

  print_row();
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value;
  print_row();
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value;
  print_row();
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value;
  print_row();
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
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

PushStateEvent::PushStateEvent (double timestamp, container_t container, type_t type, val_t value, void* extra)
{
  this->event_type                  = PAJE_PushState;
  this->timestamp                   = timestamp;
  this->type = type;
  this->container = container;
  this->value     = value;
  this->extra     = extra;

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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id;
  print_row();
}

ResetStateEvent::ResetStateEvent (double timestamp, container_t container, type_t type)
{
  this->event_type                        = PAJE_ResetState;
  this->timestamp                         = timestamp;
  this->type      = type;
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

void ResetStateEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id;
  print_row();
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
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


void EndLinkEvent::print() {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " <<type->id
         << " " <<container->id
         << " " <<value;
  stream << " " << destContainer->id
         << " " << key;
  print_row();
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
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event_type, TRACE_precision(), timestamp);
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int)this->event_type;
  print_timestamp(this);
  stream << " " << type->id
         << " " << container->id
         << " " << value->id;
  print_row();
}
