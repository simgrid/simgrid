/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "src/instr/instr_smpi.h"
#include "src/smpi/private.hpp"
#include "xbt/virtu.h" /* sg_cmdline */
#include "typeinfo"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_trace, instr, "tracing event system");

FILE *tracing_file = nullptr;

void print_NULL(paje_event_t event){}

/* The active set of functions for the selected trace format
 * By default, they all do nothing, hence the print_NULL to avoid segfaults */

s_instr_trace_writer_t active_writer = {&print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL,
                                        &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL,
                                        &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL, &print_NULL};

std::vector<paje_event_t> buffer;

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
    for (auto event :buffer){
      event->print();
      delete event;
    }
    buffer.clear();
  }else{
    std::vector<paje_event_t>::iterator i = buffer.begin();
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

/* internal do the instrumentation module */
static void insert_into_buffer (paje_event_t tbi)
{
  if (TRACE_buffer() == 0){
    tbi->print ();
    delete tbi;
    return;
  }

  XBT_DEBUG("%s: insert event_type=%d, timestamp=%f, buffersize=%zu)",
      __FUNCTION__, (int)tbi->event_type, tbi->timestamp, buffer.size());
  std::vector<paje_event_t>::reverse_iterator i;
  for (i = buffer.rbegin(); i != buffer.rend(); ++i) {
    paje_event_t e1 = *i;
    if (e1->timestamp <= tbi->timestamp)
      break;
  }
  buffer.insert(i.base(), tbi);
  if (i == buffer.rend())
    XBT_DEBUG("%s: inserted at beginning", __FUNCTION__);
  else
    XBT_DEBUG("%s: inserted at%s %zd", __FUNCTION__, (i == buffer.rbegin()) ? " end" :"pos =",
        std::distance(buffer.rend(),i));
}

paje_event:: ~paje_event()
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, timestamp);
 
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


//--------------------------------------------------------

DefineContainerEvent::DefineContainerEvent(type_t type)
{

  event_type                            = PAJE_DefineContainerType;
  timestamp                             = 0;
  this->type = type;
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);
  //print it
  print ();
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
// TODO convertir tt les constructeurs proprement.
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

DefineEntityValueEvent::DefineEntityValueEvent (val_t value)
{
  this->event_type                           = PAJE_DefineEntityValue;
  this->timestamp                            = 0;
  this->value = value;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event_type);

  //print it
  print();
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

DestroyContainerEvent::DestroyContainerEvent (container_t container)
{
  this->event_type                              = PAJE_DestroyContainer;
  this->timestamp                               = SIMIX_get_clock();
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  //print it
  print();
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
{
  new PushStateEvent(timestamp, container, type, value, nullptr);
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


ResetStateEvent::ResetStateEvent (double timestamp, container_t container, type_t type)
{
  this->event_type                        = PAJE_ResetState;
  this->timestamp                         = timestamp;
  this->type      = type;
  this->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
}

StartLinkEvent::StartLinkEvent (double timestamp, container_t container, type_t type, container_t sourceContainer,
                        const char *value, const char *key)
{
  StartLinkEvent(timestamp, container, type, sourceContainer, value, key, -1);
}

StartLinkEvent::StartLinkEvent (double timestamp, container_t container, type_t type, container_t sourceContainer,
                                const char *value, const char *key, int size)
{
  event_type                             = PAJE_StartLink;
  timestamp                              = timestamp;
  this->type            = type;
  this->container       = container;
  this->sourceContainer = sourceContainer;
  value           = xbt_strdup(value);
  key             = xbt_strdup(key);
  this->size            = size;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event_type, this->timestamp);

  insert_into_buffer (this);
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
