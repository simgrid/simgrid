/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "xbt/virtu.h" /* sg_cmdline */
#include <sstream>
#include <iomanip> /** std::setprecision **/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr_trace, "tracing event system");

extern FILE * tracing_file;
extern s_instr_trace_writer_t active_writer;

static void print_paje_debug(std::string functionName, paje_event_t event) {
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
}

template<typename T> static std::stringstream init_stream(paje_event_t event) {
  std::stringstream stream;
  stream << std::fixed << std::setprecision(TRACE_precision());
  stream << (int) event->event_type;

  return stream;
}

static void print_row(std::stringstream& stream) {
  stream << std::endl;
  fprintf(tracing_file, "%s", stream.str().c_str());
}

static void print_timestamp(std::stringstream& stream, paje_event_t event) {
  stream << " ";
  /* prevent 0.0000 in the trace - this was the behavior before the transition to c++ */
  if (event->timestamp == 0) 
    stream << 0;
  else 
    stream << event->timestamp;
}


template<typename T> static std::stringstream print_default_pajeLink_row(paje_event_t& event) {
  std::stringstream stream = init_stream<T>(event);
  print_timestamp(stream, event);
  stream << " " << static_cast<T>(event->data)->type->id
         << " " << static_cast<T>(event->data)->container->id
         << " " << static_cast<T>(event->data)->value;

  return stream;
}

template<typename T> static std::stringstream print_default_pajeState_row(paje_event_t& event) {
  std::stringstream stream = init_stream<T>(event);
  print_timestamp(stream, event);
  stream << " " << static_cast<T>(event->data)->type->id
         << " " << static_cast<T>(event->data)->container->id;
         
  return stream;
}

template<typename T> static std::stringstream print_default_pajeType_row(paje_event_t& event) {
  std::stringstream stream = init_stream<T>(event);
  stream << " " << static_cast<T>(event->data)->type->id
         << " " << static_cast<T>(event->data)->type->father->id
         << " " << static_cast<T>(event->data)->type->name;
         
  return stream;
}

template<typename T> static void print_default_pajeVariable_row(paje_event_t& event) {
  std::stringstream stream = init_stream<T>(event);
  print_timestamp(stream, event);
  stream << " " << static_cast<T>(event->data)->type->id
         << " " << static_cast<T>(event->data)->container->id
         << " " << static_cast<T>(event->data)->value;
         
  print_row(stream);
}

void TRACE_paje_init(void) {
  active_writer.print_DefineContainerType = print_pajeDefineContainerType;
  active_writer.print_DefineVariableType  = print_pajeDefineVariableType;
  active_writer.print_DefineStateType     = print_pajeDefineStateType;
  active_writer.print_DefineEventType     = print_pajeDefineEventType;
  active_writer.print_DefineLinkType      = print_pajeDefineLinkType;
  active_writer.print_DefineEntityValue   = print_pajeDefineEntityValue;
  active_writer.print_CreateContainer     = print_pajeCreateContainer;
  active_writer.print_DestroyContainer    = print_pajeDestroyContainer;
  active_writer.print_SetVariable         = print_pajeSetVariable;
  active_writer.print_AddVariable         = print_pajeAddVariable;
  active_writer.print_SubVariable         = print_pajeSubVariable;
  active_writer.print_SetState            = print_pajeSetState;
  active_writer.print_PushState           = print_pajePushState;
  active_writer.print_PopState            = print_pajePopState;
  active_writer.print_ResetState          = print_pajeResetState;
  active_writer.print_StartLink           = print_pajeStartLink;
  active_writer.print_EndLink             = print_pajeEndLink;
  active_writer.print_NewEvent            = print_pajeNewEvent;
}

void TRACE_paje_start(void) {
  char *filename = TRACE_get_filename();
  tracing_file = fopen(filename, "w");
  if (tracing_file == NULL){
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

void TRACE_paje_end(void) {
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  XBT_DEBUG("Filename %s is closed", filename);
}

void print_pajeDefineContainerType(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeType_row<defineContainerType_t>(event);
  print_row(stream);
}

void print_pajeDefineVariableType(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeType_row<defineVariableType_t>(event);
  stream << " \"" << static_cast<defineVariableType_t>(event->data)->type->color << "\"";
  print_row(stream);
}

void print_pajeDefineStateType(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeType_row<defineStateType_t>(event);
  print_row(stream);
}

void print_pajeDefineEventType(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeType_row<defineEventType_t>(event);
  print_row(stream);
}

void print_pajeDefineLinkType(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = init_stream<defineLinkType_t>(event);
  stream << " " << static_cast<defineLinkType_t>(event->data)->type->id 
         << " " << static_cast<defineLinkType_t>(event->data)->type->father->id 
         << " " << static_cast<defineLinkType_t>(event->data)->source->id 
         << " " << static_cast<defineLinkType_t>(event->data)->dest->id 
         << " " << static_cast<defineLinkType_t>(event->data)->type->name;
  print_row(stream);
}

void print_pajeDefineEntityValue (paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = init_stream<defineEntityValue_t>(event);
  stream << " "   << static_cast<defineEntityValue_t>(event->data)->value->id
         << " "   << static_cast<defineEntityValue_t>(event->data)->value->father->id
         << " "   << static_cast<defineEntityValue_t>(event->data)->value->name
         << " \"" << static_cast<defineEntityValue_t>(event->data)->value->color << "\"";
  print_row(stream);
}

void print_pajeCreateContainer(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = init_stream<createContainer_t>(event);
  print_timestamp(stream, event);
  stream << " "   << static_cast<createContainer_t>(event->data)->container->id
         << " "   << static_cast<createContainer_t>(event->data)->container->type->id
         << " "   << static_cast<createContainer_t>(event->data)->container->father->id
         << " \"" << static_cast<createContainer_t>(event->data)->container->name << "\"";

  print_row(stream);
}

void print_pajeDestroyContainer(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = init_stream<createContainer_t>(event);
  print_timestamp(stream, event);
  stream << " "   << static_cast<createContainer_t>(event->data)->container->type->id
         << " "   << static_cast<createContainer_t>(event->data)->container->id;

  print_row(stream);
}

void print_pajeSetVariable(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  print_default_pajeVariable_row<setVariable_t>(event);
}

void print_pajeAddVariable(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  print_default_pajeVariable_row<addVariable_t>(event);
}

void print_pajeSubVariable(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  print_default_pajeVariable_row<subVariable_t>(event);
}

void print_pajeSetState(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);

  std::stringstream stream = print_default_pajeState_row<setState_t>(event);
  stream << " " << static_cast<setState_t>(event->data)->value->id;
  print_row(stream);
}

void print_pajePushState(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeState_row<pushState_t>(event);
  stream << " " << static_cast<pushState_t>(event->data)->value->id;

  if (TRACE_display_sizes()) {
    stream << " ";
    if (static_cast<pushState_t>(event->data)->extra != NULL) {
      stream << static_cast<instr_extra_data>(static_cast<pushState_t>(event->data)->extra)->send_size;
    }
    else {
      stream << 0;
    }
  }
  print_row(stream);

  if (static_cast<pushState_t>(event->data)->extra != NULL) {
    if (static_cast<instr_extra_data>(static_cast<pushState_t>(event->data)->extra)->sendcounts != NULL)
      xbt_free(static_cast<instr_extra_data>(static_cast<pushState_t>(event->data)->extra)->sendcounts);
    if (static_cast<instr_extra_data>(static_cast<pushState_t>(event->data)->extra)->recvcounts != NULL)
      xbt_free(static_cast<instr_extra_data>(static_cast<pushState_t>(event->data)->extra)->recvcounts);
    xbt_free(static_cast<pushState_t>(event->data)->extra);
  }
}

void print_pajePopState(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeState_row<popState_t>(event);
  print_row(stream);
}

void print_pajeResetState(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeState_row<resetState_t>(event);
  print_row(stream);
}

void print_pajeStartLink(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeLink_row<startLink_t>(event);
  stream << " " << static_cast<startLink_t>(event->data)->sourceContainer->id
         << " " << static_cast<startLink_t>(event->data)->key;

  if (TRACE_display_sizes()) {
    stream << " " << static_cast<startLink_t>(event->data)->size;
  }
  print_row(stream);
}

void print_pajeEndLink(paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = print_default_pajeLink_row<startLink_t>(event);
  stream << " " << static_cast<endLink_t>(event->data)->destContainer->id
         << " " << static_cast<endLink_t>(event->data)->key;
  print_row(stream);
}

void print_pajeNewEvent (paje_event_t event) {
  print_paje_debug(__FUNCTION__, event);
  std::stringstream stream = init_stream<newEvent_t>(event);
  print_timestamp(stream, event);
  stream << " " << static_cast<newEvent_t>(event->data)->type->id
         << " " << static_cast<newEvent_t>(event->data)->container->id
         << " " << static_cast<newEvent_t>(event->data)->value->id;
  print_row(stream);
}
