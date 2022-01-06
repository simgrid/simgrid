/* Copyright (c) 2010-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "src/instr/instr_private.hpp"
#include "src/smpi/include/private.hpp"
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
  for (auto const& str : simgrid::xbt::cmdline) {
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
  tracing_file << "%EventDef PajeDefineContainerType " << PajeEventType::DefineContainerType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  if (basic)
    tracing_file << "%       ContainerType string" << std::endl;
  else
    tracing_file << "%       Type string" << std::endl;

  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineVariableType " << PajeEventType::DefineVariableType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%       Color color" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineStateType " << PajeEventType::DefineStateType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineEventType " << PajeEventType::DefineEventType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDefineLinkType " << PajeEventType::DefineLinkType << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string" << std::endl;
  tracing_file << "%       " << (basic ? "Source" : "Start") << "ContainerType string" << std::endl;
  tracing_file << "%       " << (basic ? "Dest" : "End") << "ContainerType string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // EntityValue
  tracing_file << "%EventDef PajeDefineEntityValue " << PajeEventType::DefineEntityValue << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       " << (basic ? "Entity" : "") << "Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%       Color color" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // Container
  tracing_file << "%EventDef PajeCreateContainer " << PajeEventType::CreateContainer << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Alias string" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeDestroyContainer " << PajeEventType::DestroyContainer << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Name string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // Variable
  tracing_file << "%EventDef PajeSetVariable " << PajeEventType::SetVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeAddVariable " << PajeEventType::AddVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeSubVariable " << PajeEventType::SubVariable << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value double" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // State
  tracing_file << "%EventDef PajeSetState " << PajeEventType::SetState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajePushState " << PajeEventType::PushState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  if (display_sizes)
    tracing_file << "%       Size int" << std::endl;
#if HAVE_SMPI
  if (smpi_cfg_trace_call_location()) {
    /* paje currently (May 2016) uses "Filename" and "Linenumber" as reserved words. We cannot use them... */
    tracing_file << "%       Fname string" << std::endl;
    tracing_file << "%       Lnumber int" << std::endl;
  }
#endif
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajePopState " << PajeEventType::PopState << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  if (not basic) {
    tracing_file << "%EventDef PajeResetState " << PajeEventType::ResetState << std::endl;
    tracing_file << "%       Time date" << std::endl;
    tracing_file << "%       Type string" << std::endl;
    tracing_file << "%       Container string" << std::endl;
    tracing_file << "%EndEventDef" << std::endl;
  }

  // Link
  tracing_file << "%EventDef PajeStartLink " << PajeEventType::StartLink << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%       " << (basic ? "Source" : "Start") << "Container string" << std::endl;
  tracing_file << "%       Key string" << std::endl;
  if (display_sizes)
    tracing_file << "%       Size int" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  tracing_file << "%EventDef PajeEndLink " << PajeEventType::EndLink << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%       " << (basic ? "Dest" : "End") << "Container string" << std::endl;
  tracing_file << "%       Key string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;

  // Event
  tracing_file << "%EventDef PajeNewEvent " << PajeEventType::NewEvent << std::endl;
  tracing_file << "%       Time date" << std::endl;
  tracing_file << "%       Type string" << std::endl;
  tracing_file << "%       Container string" << std::endl;
  tracing_file << "%       Value string" << std::endl;
  tracing_file << "%EndEventDef" << std::endl;
}
} // namespace paje
} // namespace instr
} // namespace simgrid
