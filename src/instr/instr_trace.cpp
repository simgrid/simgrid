/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "xbt/virtu.h" /* sg_cmdline */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_trace, instr, "tracing event system");

FILE *tracing_file = NULL;

void print_NULL(paje_event_t event){}

/* The active set of functions for the selected trace format
 * By default, they all do nothing, hence the print_NULL to avoid segfaults */

s_instr_trace_writer_t active_writer = {
    print_NULL, print_NULL, print_NULL, print_NULL,
    print_NULL, print_NULL, print_NULL, print_NULL,
    print_NULL, print_NULL, print_NULL, print_NULL,
    print_NULL, print_NULL, print_NULL, print_NULL,
    print_NULL, print_NULL
};

xbt_dynar_t buffer = NULL;

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

void TRACE_init()
{
  buffer = xbt_dynar_new(sizeof(paje_event_t), NULL);
}

void TRACE_finalize()
{
  xbt_dynar_free(&buffer);
}

double TRACE_last_timestamp_to_dump = 0;
//dumps the trace file until the timestamp TRACE_last_timestamp_to_dump
void TRACE_paje_dump_buffer (int force)
{
  if (!TRACE_is_enabled()) return;
  XBT_DEBUG("%s: dump until %f. starts", __FUNCTION__, TRACE_last_timestamp_to_dump);
  if (force){
    paje_event_t event;
    unsigned int i;
    xbt_dynar_foreach(buffer, i, event){
      event->print (event);
      event->free (event);
    }
    xbt_dynar_free (&buffer);
    buffer = xbt_dynar_new (sizeof(paje_event_t), NULL);
  }else{
    paje_event_t event;
    unsigned int cursor;
    xbt_dynar_foreach(buffer, cursor, event) {
      double head_timestamp = event->timestamp;
      if (head_timestamp > TRACE_last_timestamp_to_dump){
        break;
      }
      event->print (event);
      event->free (event);
    }
    xbt_dynar_remove_n_at(buffer, cursor, 0);
  }
  XBT_DEBUG("%s: ends", __FUNCTION__);
}

/* internal do the instrumentation module */
static void insert_into_buffer (paje_event_t tbi)
{
  if (TRACE_buffer() == 0){
    tbi->print (tbi);
    tbi->free (tbi);
    return;
  }

  XBT_DEBUG("%s: insert event_type=%d, timestamp=%f, buffersize=%lu)",
      __FUNCTION__, (int)tbi->event_type, tbi->timestamp, xbt_dynar_length(buffer));

  unsigned int i;
  for (i = xbt_dynar_length(buffer); i > 0; i--) {
    paje_event_t e1 = *(paje_event_t*)xbt_dynar_get_ptr(buffer, i - 1);
    if (e1->timestamp <= tbi->timestamp)
      break;
  }
  xbt_dynar_insert_at(buffer, i, &tbi);
  if (i == 0)
    XBT_DEBUG("%s: inserted at beginning", __FUNCTION__);
  else
    XBT_DEBUG("%s: inserted at%s %u", __FUNCTION__, (i == xbt_dynar_length(buffer) - 1 ? " end, pos =" : ""), i);
}

static void free_paje_event (paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);
  switch (event->event_type){
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
  }
  xbt_free (event->data);
  xbt_free (event);
}

void new_pajeDefineContainerType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineContainerType;
  event->timestamp = 0;
  event->print = active_writer.print_DefineContainerType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineContainerType_t, 1);
  ((defineContainerType_t)(event->data))->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineVariableType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineVariableType;
  event->timestamp = 0;
  event->print = active_writer.print_DefineVariableType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineVariableType_t, 1);
  ((defineVariableType_t)(event->data))->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineStateType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineStateType;
  event->timestamp = 0;
  event->print = active_writer.print_DefineStateType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineStateType_t, 1);
  ((defineStateType_t)(event->data))->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineEventType(type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineEventType;
  event->timestamp = 0;
  event->print = active_writer.print_DefineEventType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineEventType_t, 1);
  ((defineEventType_t)(event->data))->type = type;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineLinkType(type_t type, type_t source, type_t dest)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineLinkType;
  event->timestamp = 0;
  event->print = active_writer.print_DefineLinkType;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineLinkType_t, 1);
  ((defineLinkType_t)(event->data))->type = type;
  ((defineLinkType_t)(event->data))->source = source;
  ((defineLinkType_t)(event->data))->dest = dest;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDefineEntityValue (val_t value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DefineEntityValue;
  event->timestamp = 0;
  event->print = active_writer.print_DefineEntityValue;
  event->free = free_paje_event;
  event->data = xbt_new0(s_defineEntityValue_t, 1);
  ((defineEntityValue_t)(event->data))->value = value;

  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeCreateContainer (container_t container)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_CreateContainer;
  event->timestamp = SIMIX_get_clock();
  event->print = active_writer.print_CreateContainer;
  event->free = free_paje_event;
  event->data = xbt_new0(s_createContainer_t, 1);
  ((createContainer_t)(event->data))->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeDestroyContainer (container_t container)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_DestroyContainer;
  event->timestamp = SIMIX_get_clock();
  event->print = active_writer.print_DestroyContainer;
  event->free = free_paje_event;
  event->data = xbt_new0(s_destroyContainer_t, 1);
  ((destroyContainer_t)(event->data))->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  //print it
  event->print (event);
  event->free (event);
}

void new_pajeSetVariable (double timestamp, container_t container, type_t type, double value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_SetVariable;
  event->timestamp = timestamp;
  event->print = active_writer.print_SetVariable;
  event->free = free_paje_event;
  event->data = xbt_new0(s_setVariable_t, 1);
  ((setVariable_t)(event->data))->type = type;
  ((setVariable_t)(event->data))->container = container;
  ((setVariable_t)(event->data))->value = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}


void new_pajeAddVariable (double timestamp, container_t container, type_t type, double value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_AddVariable;
  event->timestamp = timestamp;
  event->print = active_writer.print_AddVariable;
  event->free = free_paje_event;
  event->data = xbt_new0(s_addVariable_t, 1);
  ((addVariable_t)(event->data))->type = type;
  ((addVariable_t)(event->data))->container = container;
  ((addVariable_t)(event->data))->value = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajeSubVariable (double timestamp, container_t container, type_t type, double value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_SubVariable;
  event->timestamp = timestamp;
  event->print = active_writer.print_SubVariable;
  event->free = free_paje_event;
  event->data = xbt_new0(s_subVariable_t, 1);
  ((subVariable_t)(event->data))->type = type;
  ((subVariable_t)(event->data))->container = container;
  ((subVariable_t)(event->data))->value = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajeSetState (double timestamp, container_t container, type_t type, val_t value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_SetState;
  event->timestamp = timestamp;
  event->print = active_writer.print_SetState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_setState_t, 1);
  ((setState_t)(event->data))->type = type;
  ((setState_t)(event->data))->container = container;
  ((setState_t)(event->data))->value = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}


void new_pajePushStateWithExtra (double timestamp, container_t container, type_t type, val_t value, void* extra)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_PushState;
  event->timestamp = timestamp;
  event->print = active_writer.print_PushState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_pushState_t, 1);
  ((pushState_t)(event->data))->type = type;
  ((pushState_t)(event->data))->container = container;
  ((pushState_t)(event->data))->value = value;
  ((pushState_t)(event->data))->extra = extra;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}


void new_pajePushState (double timestamp, container_t container, type_t type, val_t value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_PushState;
  event->timestamp = timestamp;
  event->print = active_writer.print_PushState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_pushState_t, 1);
  ((pushState_t)(event->data))->type = type;
  ((pushState_t)(event->data))->container = container;
  ((pushState_t)(event->data))->value = value;
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajePopState (double timestamp, container_t container, type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_PopState;
  event->timestamp = timestamp;
  event->print = active_writer.print_PopState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_popState_t, 1);
  ((popState_t)(event->data))->type = type;
  ((popState_t)(event->data))->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}


void new_pajeResetState (double timestamp, container_t container, type_t type)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_ResetState;
  event->timestamp = timestamp;
  event->print = active_writer.print_ResetState;
  event->free = free_paje_event;
  event->data = xbt_new0(s_resetState_t, 1);
  ((resetState_t)(event->data))->type = type;
  ((resetState_t)(event->data))->container = container;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajeStartLink (double timestamp, container_t container, type_t type, container_t sourceContainer,
                        const char *value, const char *key)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_StartLink;
  event->timestamp = timestamp;
  event->print = active_writer.print_StartLink;
  event->free = free_paje_event;
  event->data = xbt_new0(s_startLink_t, 1);
  ((startLink_t)(event->data))->type = type;
  ((startLink_t)(event->data))->container = container;
  ((startLink_t)(event->data))->sourceContainer = sourceContainer;
  ((startLink_t)(event->data))->value = xbt_strdup(value);
  ((startLink_t)(event->data))->key = xbt_strdup(key);
  ((startLink_t)(event->data))->size = -1;
  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajeStartLinkWithSize (double timestamp, container_t container, type_t type, container_t sourceContainer,
                                const char *value, const char *key, int size)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_StartLink;
  event->timestamp = timestamp;
  event->print = active_writer.print_StartLink;
  event->free = free_paje_event;
  event->data = xbt_new0(s_startLink_t, 1);
  ((startLink_t)(event->data))->type = type;
  ((startLink_t)(event->data))->container = container;
  ((startLink_t)(event->data))->sourceContainer = sourceContainer;
  ((startLink_t)(event->data))->value = xbt_strdup(value);
  ((startLink_t)(event->data))->key = xbt_strdup(key);
  ((startLink_t)(event->data))->size = size;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajeEndLink (double timestamp, container_t container, type_t type, container_t destContainer,
                      const char *value, const char *key)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_EndLink;
  event->timestamp = timestamp;
  event->print = active_writer.print_EndLink;
  event->free = free_paje_event;
  event->data = xbt_new0(s_endLink_t, 1);
  ((endLink_t)(event->data))->type = type;
  ((endLink_t)(event->data))->container = container;
  ((endLink_t)(event->data))->destContainer = destContainer;
  ((endLink_t)(event->data))->value = xbt_strdup(value);
  ((endLink_t)(event->data))->key = xbt_strdup(key);

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}

void new_pajeNewEvent (double timestamp, container_t container, type_t type, val_t value)
{
  paje_event_t event = xbt_new0(s_paje_event_t, 1);
  event->event_type = PAJE_NewEvent;
  event->timestamp = timestamp;
  event->print = active_writer.print_NewEvent;
  event->free = free_paje_event;
  event->data = xbt_new0(s_newEvent_t, 1);
  ((newEvent_t)(event->data))->type = type;
  ((newEvent_t)(event->data))->container = container;
  ((newEvent_t)(event->data))->value = value;

  XBT_DEBUG("%s: event_type=%d, timestamp=%f", __FUNCTION__, (int)event->event_type, event->timestamp);

  insert_into_buffer (event);
}
