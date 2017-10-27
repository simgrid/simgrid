/* Copyright (c) 2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.h"
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_paje_header, instr, "Paje tracing event system (header)");

extern FILE *tracing_file;

static void TRACE_header_PajeDefineContainerType(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeDefineContainerType %u\n", simgrid::instr::PAJE_DefineContainerType);
  fprintf(tracing_file, "%%       Alias string\n");
  if (basic){
    fprintf(tracing_file, "%%       ContainerType string\n");
  }else{
    fprintf(tracing_file, "%%       Type string\n");
  }
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeDefineVariableType(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeDefineVariableType %u\n", simgrid::instr::PAJE_DefineVariableType);
  fprintf(tracing_file, "%%       Alias string\n");
  if (basic){
    fprintf(tracing_file, "%%       ContainerType string\n");
  }else{
    fprintf(tracing_file, "%%       Type string\n");
  }
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%       Color color\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeDefineStateType(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeDefineStateType %u\n", simgrid::instr::PAJE_DefineStateType);
  fprintf(tracing_file, "%%       Alias string\n");
  if (basic){
    fprintf(tracing_file, "%%       ContainerType string\n");
  }else{
    fprintf(tracing_file, "%%       Type string\n");
  }
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeDefineEventType(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeDefineEventType %u\n", simgrid::instr::PAJE_DefineEventType);
  fprintf(tracing_file, "%%       Alias string\n");
  if (basic){
    fprintf(tracing_file, "%%       ContainerType string\n");
  }else{
    fprintf(tracing_file, "%%       Type string\n");
  }
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeDefineLinkType(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeDefineLinkType %u\n", simgrid::instr::PAJE_DefineLinkType);
  fprintf(tracing_file, "%%       Alias string\n");
  if (basic){
    fprintf(tracing_file, "%%       ContainerType string\n");
    fprintf(tracing_file, "%%       SourceContainerType string\n");
    fprintf(tracing_file, "%%       DestContainerType string\n");
  }else{
    fprintf(tracing_file, "%%       Type string\n");
    fprintf(tracing_file, "%%       StartContainerType string\n");
    fprintf(tracing_file, "%%       EndContainerType string\n");
  }
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeDefineEntityValue(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeDefineEntityValue %u\n", simgrid::instr::PAJE_DefineEntityValue);
  fprintf(tracing_file, "%%       Alias string\n");
  if (basic){
    fprintf(tracing_file, "%%       EntityType string\n");
  }else{
    fprintf(tracing_file, "%%       Type string\n");
  }
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%       Color color\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeCreateContainer()
{
  fprintf(tracing_file, "%%EventDef PajeCreateContainer %u\n", simgrid::instr::PAJE_CreateContainer);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Alias string\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeDestroyContainer()
{
  fprintf(tracing_file, "%%EventDef PajeDestroyContainer %u\n", simgrid::instr::PAJE_DestroyContainer);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Name string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeSetVariable()
{
  fprintf(tracing_file, "%%EventDef PajeSetVariable %u\n", simgrid::instr::PAJE_SetVariable);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value double\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeAddVariable()
{
  fprintf(tracing_file, "%%EventDef PajeAddVariable %u\n", simgrid::instr::PAJE_AddVariable);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value double\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeSubVariable()
{
  fprintf(tracing_file, "%%EventDef PajeSubVariable %u\n", simgrid::instr::PAJE_SubVariable);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value double\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeSetState()
{
  fprintf(tracing_file, "%%EventDef PajeSetState %u\n", simgrid::instr::PAJE_SetState);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajePushState(int size)
{
  fprintf(tracing_file, "%%EventDef PajePushState %u\n", simgrid::instr::PAJE_PushState);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value string\n");
  if (size) fprintf(tracing_file, "%%       Size int\n");
#if HAVE_SMPI
  if (xbt_cfg_get_boolean("smpi/trace-call-location")) {
    /**
     * paje currently (May 2016) uses "Filename" and "Linenumber" as
     * reserved words. We cannot use them...
     */
    fprintf(tracing_file, "%%       Fname string\n");
    fprintf(tracing_file, "%%       Lnumber int\n");
  }
#endif
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajePopState()
{
  fprintf(tracing_file, "%%EventDef PajePopState %u\n", simgrid::instr::PAJE_PopState);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeResetState(bool basic)
{
  if (basic)
    return;

  fprintf(tracing_file, "%%EventDef PajeResetState %u\n", simgrid::instr::PAJE_ResetState);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeStartLink(bool basic, int size)
{
  fprintf(tracing_file, "%%EventDef PajeStartLink %u\n", simgrid::instr::PAJE_StartLink);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value string\n");
  if (basic){
    fprintf(tracing_file, "%%       SourceContainer string\n");
  }else{
    fprintf(tracing_file, "%%       StartContainer string\n");
  }
  fprintf(tracing_file, "%%       Key string\n");
  if (size) fprintf(tracing_file, "%%       Size int\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeEndLink(bool basic)
{
  fprintf(tracing_file, "%%EventDef PajeEndLink %u\n", simgrid::instr::PAJE_EndLink);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value string\n");
  if (basic){
    fprintf(tracing_file, "%%       DestContainer string\n");
  }else{
    fprintf(tracing_file, "%%       EndContainer string\n");
  }
  fprintf(tracing_file, "%%       Key string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

static void TRACE_header_PajeNewEvent()
{
  fprintf(tracing_file, "%%EventDef PajeNewEvent %u\n", simgrid::instr::PAJE_NewEvent);
  fprintf(tracing_file, "%%       Time date\n");
  fprintf(tracing_file, "%%       Type string\n");
  fprintf(tracing_file, "%%       Container string\n");
  fprintf(tracing_file, "%%       Value string\n");
  fprintf(tracing_file, "%%EndEventDef\n");
}

void TRACE_header(bool basic, int size)
{
  XBT_DEBUG ("Define paje header");
  TRACE_header_PajeDefineContainerType(basic);
  TRACE_header_PajeDefineVariableType(basic);
  TRACE_header_PajeDefineStateType(basic);
  TRACE_header_PajeDefineEventType(basic);
  TRACE_header_PajeDefineLinkType(basic);
  TRACE_header_PajeDefineEntityValue(basic);
  TRACE_header_PajeCreateContainer();
  TRACE_header_PajeDestroyContainer();
  TRACE_header_PajeSetVariable();
  TRACE_header_PajeAddVariable();
  TRACE_header_PajeSubVariable();
  TRACE_header_PajeSetState();
  TRACE_header_PajePushState(size);
  TRACE_header_PajePopState();
  TRACE_header_PajeResetState(basic);
  TRACE_header_PajeStartLink (basic, size);
  TRACE_header_PajeEndLink(basic);
  TRACE_header_PajeNewEvent();
}
