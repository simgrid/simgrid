/* Copyright (c) 2010-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "src/instr/instr_private.hpp"
#include "xbt/virtu.h" /* xbt::cmdline */

extern std::ofstream tracing_file;
namespace simgrid {
namespace instr {
namespace paje {

void dump_generator_version()
{
  tracing_file << "#This file was generated using SimGrid-" << SIMGRID_VERSION_MAJOR << "." << SIMGRID_VERSION_MINOR
               << "." << SIMGRID_VERSION_PATCH << std::endl;
  tracing_file << "#[";
  for (auto str : simgrid::xbt::cmdline) {
    tracing_file << str << " ";
  }
  tracing_file << "]" << std::endl;
}

void dump_comment_file(const std::string& filename)
{
  if (filename.empty())
    return;
  std::ifstream fs(filename.c_str(), std::ifstream::in);

  if (fs.fail())
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("Comment file %s could not be opened for reading.", filename.c_str()));

  while (not fs.eof()) {
    std::string line;
    std::getline(fs, line);
    tracing_file << "# " << line;
  }
  fs.close();
}

void dump_header(bool basic, bool display_sizes)
{
  // Types
  tracing_file << "%EventDef PajeDefineContainerType " << PAJE_DefineContainerType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic)
    tracing_file << "%       ContainerType string" << std::endl;
  else
    tracing_file << "%       Type string" << std::endl;

  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineVariableType " << PAJE_DefineVariableType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%       Color color" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineStateType " << PAJE_DefineStateType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineEventType " << PAJE_DefineEventType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineLinkType " << PAJE_DefineLinkType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       " << (basic ? "Source" : "Start") << "ContainerType string" << std::endl;
  tracing_file << "%       " << (basic ? "Dest" : "End") << "ContainerType string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // EntityValue
  tracing_file << "%EventDef PajeDefineEntityValue " << PAJE_DefineEntityValue << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Entity" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%       Color color" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // Container
  tracing_file << "%EventDef PajeCreateContainer " << PAJE_CreateContainer << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDestroyContainer " << PAJE_DestroyContainer << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // Variable
  tracing_file << "%EventDef PajeSetVariable " << PAJE_SetVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeAddVariable " << PAJE_AddVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeSubVariable " << PAJE_SubVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // State
  tracing_file << "%EventDef PajeSetState " << PAJE_SetState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajePushState " << PAJE_PushState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  if (display_sizes)
    tracing_file << "%       Size int" << std::endl;
#if HAVE_SMPI
  if (simgrid::config::get_value<bool>("smpi/trace-call-location")) {
    /* paje currently (May 2016) uses "Filename" and "Linenumber" as reserved words. We cannot use them... */
    tracing_file << "%       Fname string" << std::endl;
    tracing_file << "%       Lnumber int" << std::endl;
  }
#endif
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajePopState " << PAJE_PopState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  if (not basic) {
    tracing_file << "%EventDef PajeResetState " << PAJE_ResetState << std::endl;
    tracing_file << "%       Time date" << std::endl;
    tracing_file << "%       Type string" << std::endl;
    tracing_file << "%       Container string" << std::endl;
    tracing_file << "%EndEventDef" << std::endl;
  }

  // Link
  tracing_file << "%EventDef PajeStartLink " << PAJE_StartLink << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%       " << (basic ? "Source" : "Start") << "Container string" << std::endl;
  tracing_file << "%       Key string" << std::endl;
  if (display_sizes)
    tracing_file << "%       Size int" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeEndLink " << PAJE_EndLink << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%       " << (basic ? "Dest" : "End") << "Container string" << std::endl;
  tracing_file << "%       Key string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // Event
  tracing_file << "%EventDef PajeNewEvent " << PAJE_NewEvent << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}
} // namespace paje
} // namespace instr
} // namespace simgrid
