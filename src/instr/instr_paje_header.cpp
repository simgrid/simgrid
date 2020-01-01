/* Copyright (c) 2010-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.hpp"
#include "src/instr/instr_private.hpp"

extern std::ofstream tracing_file;

static void TRACE_header_PajeDefineContainerType(bool basic)
{
  tracing_file << "%EventDef PajeDefineContainerType " << simgrid::instr::PAJE_DefineContainerType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic){
    tracing_file << "%       ContainerType string" << std::endl;
  }else{
    tracing_file << "%       Type string" << std::endl;
  }
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeDefineVariableType(bool basic)
{
  tracing_file << "%EventDef PajeDefineVariableType " << simgrid::instr::PAJE_DefineVariableType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic){
    tracing_file << "%       ContainerType string" << std::endl;
  }else{
    tracing_file << "%       Type string" << std::endl;
  }
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%       Color color" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeDefineStateType(bool basic)
{
  tracing_file << "%EventDef PajeDefineStateType " << simgrid::instr::PAJE_DefineStateType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic){
    tracing_file << "%       ContainerType string" << std::endl;
  }else{
    tracing_file << "%       Type string" << std::endl;
  }
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeDefineEventType(bool basic)
{
  tracing_file << "%EventDef PajeDefineEventType " << simgrid::instr::PAJE_DefineEventType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic){
    tracing_file << "%       ContainerType string" << std::endl;
  }else{
    tracing_file << "%       Type string" << std::endl;
  }
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeDefineLinkType(bool basic)
{
  tracing_file << "%EventDef PajeDefineLinkType " << simgrid::instr::PAJE_DefineLinkType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic){
    tracing_file << "%       ContainerType string" << std::endl;
    tracing_file << "%       SourceContainerType string" << std::endl;
    tracing_file << "%       DestContainerType string" << std::endl;
  }else{
    tracing_file << "%       Type string" << std::endl;
    tracing_file << "%       StartContainerType string" << std::endl;
    tracing_file << "%       EndContainerType string" << std::endl;
  }
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeDefineEntityValue(bool basic)
{
  tracing_file << "%EventDef PajeDefineEntityValue " << simgrid::instr::PAJE_DefineEntityValue << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic){
    tracing_file << "%       EntityType string" << std::endl;
  }else{
    tracing_file << "%       Type string" << std::endl;
  }
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%       Color color" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeCreateContainer()
{
  tracing_file << "%EventDef PajeCreateContainer " << simgrid::instr::PAJE_CreateContainer << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeDestroyContainer()
{
  tracing_file << "%EventDef PajeDestroyContainer " << simgrid::instr::PAJE_DestroyContainer << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeSetVariable()
{
  tracing_file << "%EventDef PajeSetVariable " << simgrid::instr::PAJE_SetVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeAddVariable()
{
  tracing_file << "%EventDef PajeAddVariable " << simgrid::instr::PAJE_AddVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeSubVariable()
{
  tracing_file << "%EventDef PajeSubVariable " << simgrid::instr::PAJE_SubVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeSetState()
{
  tracing_file << "%EventDef PajeSetState " << simgrid::instr::PAJE_SetState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajePushState(int size)
{
  tracing_file << "%EventDef PajePushState " << simgrid::instr::PAJE_PushState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  if (size)
    tracing_file << "%       Size int" << std::endl;
#if HAVE_SMPI
  if (simgrid::config::get_value<bool>("smpi/trace-call-location")) {
    /**
     * paje currently (May 2016) uses "Filename" and "Linenumber" as
     * reserved words. We cannot use them...
     */
    tracing_file << "%       Fname string" << std::endl;
    tracing_file << "%       Lnumber int" << std::endl;
  }
#endif
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajePopState()
{
  tracing_file << "%EventDef PajePopState " << simgrid::instr::PAJE_PopState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeResetState(bool basic)
{
  if (basic)
    return;

  tracing_file << "%EventDef PajeResetState " << simgrid::instr::PAJE_ResetState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeStartLink(bool basic, bool size)
{
  tracing_file << "%EventDef PajeStartLink " << simgrid::instr::PAJE_StartLink << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  if (basic){
    tracing_file << "%       SourceContainer string" << std::endl;
  }else{
    tracing_file << "%       StartContainer string" << std::endl;
  }
  tracing_file << "%       Key string" << std::endl;
  if (size)
    tracing_file << "%       Size int" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeEndLink(bool basic)
{
  tracing_file << "%EventDef PajeEndLink " << simgrid::instr::PAJE_EndLink << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  if (basic){
    tracing_file << "%       DestContainer string" << std::endl;
  }else{
    tracing_file << "%       EndContainer string" << std::endl;
  }
  tracing_file << "%       Key string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

static void TRACE_header_PajeNewEvent()
{
  tracing_file << "%EventDef PajeNewEvent " << simgrid::instr::PAJE_NewEvent << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}

void TRACE_header(bool basic, bool size)
{
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
