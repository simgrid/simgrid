/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "xbt/virtu.h" /* sg_cmdline */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_trace, instr_trace, "tracing event system");

extern FILE * tracing_file;
extern s_instr_trace_writer_t active_writer;

void TRACE_paje_init(void)
{
  active_writer.print_DefineContainerType=print_pajeDefineContainerType;
  active_writer.print_DefineVariableType=print_pajeDefineVariableType;
  active_writer.print_DefineStateType=print_pajeDefineStateType;
  active_writer.print_DefineEventType=print_pajeDefineEventType;
  active_writer.print_DefineLinkType=print_pajeDefineLinkType;
  active_writer.print_DefineEntityValue=print_pajeDefineEntityValue;
  active_writer.print_CreateContainer=print_pajeCreateContainer;
  active_writer.print_DestroyContainer=print_pajeDestroyContainer;
  active_writer.print_SetVariable=print_pajeSetVariable;
  active_writer.print_AddVariable=print_pajeAddVariable;
  active_writer.print_SubVariable=print_pajeSubVariable;
  active_writer.print_SetState=print_pajeSetState;
  active_writer.print_PushState=print_pajePushState;
  active_writer.print_PopState=print_pajePopState;
  active_writer.print_ResetState=print_pajeResetState;
  active_writer.print_StartLink=print_pajeStartLink;
  active_writer.print_EndLink=print_pajeEndLink;
  active_writer.print_NewEvent=print_pajeNewEvent;
}

void TRACE_paje_start(void)
{
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

void TRACE_paje_end(void)
{
  fclose(tracing_file);
  char *filename = TRACE_get_filename();
  XBT_DEBUG("Filename %s is closed", filename);
}

void print_pajeDefineContainerType(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);
  fprintf(tracing_file, "%d %s %s %s\n", (int)event->event_type, ((defineContainerType_t)event->data)->type->id,
      ((defineContainerType_t)event->data)->type->father->id, ((defineContainerType_t)event->data)->type->name);
}

void print_pajeDefineVariableType(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);
  fprintf(tracing_file, "%d %s %s %s \"%s\"\n", (int)event->event_type,
      ((defineVariableType_t)event->data)->type->id, ((defineVariableType_t)event->data)->type->father->id,
      ((defineVariableType_t)event->data)->type->name, ((defineVariableType_t)event->data)->type->color);
}

void print_pajeDefineStateType(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);
  fprintf(tracing_file, "%d %s %s %s\n", (int)event->event_type, ((defineStateType_t)event->data)->type->id,
          ((defineStateType_t)event->data)->type->father->id, ((defineStateType_t)event->data)->type->name);
}

void print_pajeDefineEventType(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);
  fprintf(tracing_file, "%d %s %s %s\n", (int)event->event_type, ((defineEventType_t)event->data)->type->id,
      ((defineEventType_t)event->data)->type->father->id, ((defineEventType_t)event->data)->type->name);
}

void print_pajeDefineLinkType(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);
  fprintf(tracing_file, "%d %s %s %s %s %s\n", (int)event->event_type, ((defineLinkType_t)event->data)->type->id,
      ((defineLinkType_t)event->data)->type->father->id, ((defineLinkType_t)event->data)->source->id,
      ((defineLinkType_t)event->data)->dest->id, ((defineLinkType_t)event->data)->type->name);
}

void print_pajeDefineEntityValue (paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d", __FUNCTION__, (int)event->event_type);
  fprintf(tracing_file, "%d %s %s %s \"%s\"\n", (int)event->event_type, ((defineEntityValue_t)event->data)->value->id,
      ((defineEntityValue_t)event->data)->value->father->id, ((defineEntityValue_t)event->data)->value->name,
      ((defineEntityValue_t)event->data)->value->color);
}

void print_pajeCreateContainer(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s \"%s\"\n", (int)event->event_type,
        ((createContainer_t)event->data)->container->id, ((createContainer_t)event->data)->container->type->id,
        ((createContainer_t)event->data)->container->father->id, ((createContainer_t)event->data)->container->name);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s %s \"%s\"\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((createContainer_t)event->data)->container->id, ((createContainer_t)event->data)->container->type->id,
        ((createContainer_t)event->data)->container->father->id, ((createContainer_t)event->data)->container->name);
  }
}

void print_pajeDestroyContainer(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n", (int)event->event_type,
        ((destroyContainer_t)event->data)->container->type->id, ((destroyContainer_t)event->data)->container->id);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((destroyContainer_t)event->data)->container->type->id, ((destroyContainer_t)event->data)->container->id);
  }
}

void print_pajeSetVariable(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n", (int)event->event_type, ((setVariable_t)event->data)->type->id,
        ((setVariable_t)event->data)->container->id, ((setVariable_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s %f\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((setVariable_t)event->data)->type->id, ((setVariable_t)event->data)->container->id,
        ((setVariable_t)event->data)->value);
  }
}

void print_pajeAddVariable(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n", (int)event->event_type, ((addVariable_t)event->data)->type->id,
        ((addVariable_t)event->data)->container->id, ((addVariable_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s %f\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((addVariable_t)event->data)->type->id, ((addVariable_t)event->data)->container->id,
        ((addVariable_t)event->data)->value);
  }
}

void print_pajeSubVariable(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %f\n", (int)event->event_type, ((subVariable_t)event->data)->type->id,
        ((subVariable_t)event->data)->container->id, ((subVariable_t)event->data)->value);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s %f\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((subVariable_t)event->data)->type->id, ((subVariable_t)event->data)->container->id,
        ((subVariable_t)event->data)->value);
  }
}

void print_pajeSetState(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n", (int)event->event_type, ((setState_t)event->data)->type->id,
        ((setState_t)event->data)->container->id, ((setState_t)event->data)->value->id);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((setState_t)event->data)->type->id, ((setState_t)event->data)->container->id,
        ((setState_t)event->data)->value->id);
  }
}

void print_pajePushState(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (!TRACE_display_sizes()){
    if (event->timestamp == 0){
      fprintf(tracing_file, "%d 0 %s %s %s\n", (int)event->event_type, ((pushState_t)event->data)->type->id,
          ((pushState_t)event->data)->container->id, ((pushState_t)event->data)->value->id);
    }else{
      fprintf(tracing_file, "%d %.*f %s %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
          ((pushState_t)event->data)->type->id, ((pushState_t)event->data)->container->id,
          ((pushState_t)event->data)->value->id);
    }
  }else{
    if (event->timestamp == 0){
      fprintf(tracing_file, "%d 0 %s %s %s ", (int)event->event_type, ((pushState_t)event->data)->type->id,
          ((pushState_t)event->data)->container->id, ((pushState_t)event->data)->value->id);
      if(((pushState_t)event->data)->extra !=NULL){
        fprintf(tracing_file, "%d ", ((instr_extra_data)((pushState_t)event->data)->extra)->send_size);
      }else{
        fprintf(tracing_file, "0 ");
      }
      fprintf(tracing_file, "\n");

    }else{
      fprintf(tracing_file, "%d %.*f %s %s %s ", (int)event->event_type, TRACE_precision(), event->timestamp,
          ((pushState_t)event->data)->type->id, ((pushState_t)event->data)->container->id,
          ((pushState_t)event->data)->value->id);
      if(((pushState_t)event->data)->extra !=NULL){
        fprintf(tracing_file, "%d ", ((instr_extra_data)((pushState_t)event->data)->extra)->send_size);
      }else{
        fprintf(tracing_file, "0 ");
      }
      fprintf(tracing_file, "\n");
    }
  }
   if(((pushState_t)event->data)->extra!=NULL){
     if(((instr_extra_data)((pushState_t)event->data)->extra)->sendcounts!=NULL)
       xbt_free(((instr_extra_data)((pushState_t)event->data)->extra)->sendcounts);
     if(((instr_extra_data)((pushState_t)event->data)->extra)->recvcounts!=NULL)
       xbt_free(((instr_extra_data)((pushState_t)event->data)->extra)->recvcounts);
     xbt_free(((pushState_t)event->data)->extra);
   }
}

void print_pajePopState(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n", (int)event->event_type, ((popState_t)event->data)->type->id,
        ((popState_t)event->data)->container->id);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((popState_t)event->data)->type->id, ((popState_t)event->data)->container->id);
  }
}

void print_pajeResetState(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s\n", (int)event->event_type, ((resetState_t)event->data)->type->id,
        ((resetState_t)event->data)->container->id);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((resetState_t)event->data)->type->id, ((resetState_t)event->data)->container->id);
  }
}

void print_pajeStartLink(paje_event_t event)
{
  if (!TRACE_display_sizes()){
    if (event->timestamp == 0){
      fprintf(tracing_file, "%d 0 %s %s %s %s %s\n", (int)event->event_type, ((startLink_t)event->data)->type->id,
          ((startLink_t)event->data)->container->id, ((startLink_t)event->data)->value,
          ((startLink_t)event->data)->sourceContainer->id, ((startLink_t)event->data)->key);
    }else {
      fprintf(tracing_file, "%d %.*f %s %s %s %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
          ((startLink_t)event->data)->type->id, ((startLink_t)event->data)->container->id,
          ((startLink_t)event->data)->value, ((startLink_t)event->data)->sourceContainer->id,
          ((startLink_t)event->data)->key);
    }
  }else{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
    if (event->timestamp == 0){
      fprintf(tracing_file, "%d 0 %s %s %s %s %s %d\n", (int)event->event_type, ((startLink_t)event->data)->type->id,
          ((startLink_t)event->data)->container->id, ((startLink_t)event->data)->value,
          ((startLink_t)event->data)->sourceContainer->id, ((startLink_t)event->data)->key,
          ((startLink_t)event->data)->size);
    }else {
      fprintf(tracing_file, "%d %.*f %s %s %s %s %s %d\n", (int)event->event_type, TRACE_precision(), event->timestamp,
          ((startLink_t)event->data)->type->id, ((startLink_t)event->data)->container->id,
          ((startLink_t)event->data)->value, ((startLink_t)event->data)->sourceContainer->id,
          ((startLink_t)event->data)->key, ((startLink_t)event->data)->size);
    }
  }
}

void print_pajeEndLink(paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s %s %s\n", (int)event->event_type, ((endLink_t)event->data)->type->id,
        ((endLink_t)event->data)->container->id, ((endLink_t)event->data)->value,
        ((endLink_t)event->data)->destContainer->id, ((endLink_t)event->data)->key);
  }else {
    fprintf(tracing_file, "%d %.*f %s %s %s %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((endLink_t)event->data)->type->id, ((endLink_t)event->data)->container->id, ((endLink_t)event->data)->value,
        ((endLink_t)event->data)->destContainer->id, ((endLink_t)event->data)->key);
  }
}

void print_pajeNewEvent (paje_event_t event)
{
  XBT_DEBUG("%s: event_type=%d, timestamp=%.*f", __FUNCTION__, (int)event->event_type, TRACE_precision(),
            event->timestamp);
  if (event->timestamp == 0){
    fprintf(tracing_file, "%d 0 %s %s %s\n", (int)event->event_type, ((newEvent_t)event->data)->type->id,
        ((newEvent_t)event->data)->container->id, ((newEvent_t)event->data)->value->id);
  }else{
    fprintf(tracing_file, "%d %.*f %s %s %s\n", (int)event->event_type, TRACE_precision(), event->timestamp,
        ((newEvent_t)event->data)->type->id, ((newEvent_t)event->data)->container->id,
        ((newEvent_t)event->data)->value->id);
  }
}
