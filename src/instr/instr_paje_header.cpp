/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/version.h"
#include "src/instr/instr_private.hpp"
#include "src/smpi/include/private.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Engine.hpp>

extern std::ofstream tracing_file;
namespace simgrid::instr::paje {

void dump_generator_version()
{
  tracing_file << "#This file was generated using SimGrid-" << SIMGRID_VERSION_MAJOR << "." << SIMGRID_VERSION_MINOR
               << "." << SIMGRID_VERSION_PATCH << '\n';
  tracing_file << "#[";
  for (auto const& str : simgrid::s4u::Engine::get_instance()->get_cmdline()) {
    tracing_file << str << " ";
  }
  tracing_file << "]\n";
}

void dump_comment_file(const std::string& filename)
{
  if (filename.empty())
    return;
  std::ifstream fs(filename.c_str(), std::ifstream::in);

  if (fs.fail())
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("Comment file %s could not be opened for reading.", filename.c_str()));

  std::string line;
  while (std::getline(fs, line))
    tracing_file << "# " << line;
  fs.close();
}

void dump_header(bool basic, bool display_sizes)
{
  // Types
  tracing_file << "%EventDef PajeDefineContainerType " << PajeEventType::DefineContainerType << '\n';
  tracing_file << "%       Alias string\n";
  if (basic)
    tracing_file << "%       ContainerType string\n";
  else
    tracing_file << "%       Type string\n";

  tracing_file << "%       Name string\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeDefineVariableType " << PajeEventType::DefineVariableType << '\n';
  tracing_file << "%       Alias string\n";
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%       Color color\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeDefineStateType " << PajeEventType::DefineStateType << '\n';
  tracing_file << "%       Alias string\n";
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeDefineEventType " << PajeEventType::DefineEventType << '\n';
  tracing_file << "%       Alias string\n";
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeDefineLinkType " << PajeEventType::DefineLinkType << '\n';
  tracing_file << "%       Alias string\n";
  tracing_file << "%       " << (basic ? "Container" : "") << "Type string\n";
  tracing_file << "%       " << (basic ? "Source" : "Start") << "ContainerType string\n";
  tracing_file << "%       " << (basic ? "Dest" : "End") << "ContainerType string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%EndEventDef\n";

  // EntityValue
  tracing_file << "%EventDef PajeDefineEntityValue " << PajeEventType::DefineEntityValue << '\n';
  tracing_file << "%       Alias string\n";
  tracing_file << "%       " << (basic ? "Entity" : "") << "Type string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%       Color color\n";
  tracing_file << "%EndEventDef\n";

  // Container
  tracing_file << "%EventDef PajeCreateContainer " << PajeEventType::CreateContainer << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Alias string\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeDestroyContainer " << PajeEventType::DestroyContainer << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Name string\n";
  tracing_file << "%EndEventDef\n";

  // Variable
  tracing_file << "%EventDef PajeSetVariable " << PajeEventType::SetVariable << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value double\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeAddVariable " << PajeEventType::AddVariable << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value double\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeSubVariable " << PajeEventType::SubVariable << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value double\n";
  tracing_file << "%EndEventDef\n";

  // State
  tracing_file << "%EventDef PajeSetState " << PajeEventType::SetState << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value string\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajePushState " << PajeEventType::PushState << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value string\n";
  if (display_sizes)
    tracing_file << "%       Size int\n";
#if HAVE_SMPI
  if (smpi_cfg_trace_call_location()) {
    /* paje currently (May 2016) uses "Filename" and "Linenumber" as reserved words. We cannot use them... */
    tracing_file << "%       Fname string\n";
    tracing_file << "%       Lnumber int\n";
  }
#endif
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajePopState " << PajeEventType::PopState << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%EndEventDef\n";

  if (not basic) {
    tracing_file << "%EventDef PajeResetState " << PajeEventType::ResetState << '\n';
    tracing_file << "%       Time date\n";
    tracing_file << "%       Type string\n";
    tracing_file << "%       Container string\n";
    tracing_file << "%EndEventDef\n";
  }

  // Link
  tracing_file << "%EventDef PajeStartLink " << PajeEventType::StartLink << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value string\n";
  tracing_file << "%       " << (basic ? "Source" : "Start") << "Container string\n";
  tracing_file << "%       Key string\n";
  if (display_sizes)
    tracing_file << "%       Size int\n";
  tracing_file << "%EndEventDef\n";

  tracing_file << "%EventDef PajeEndLink " << PajeEventType::EndLink << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value string\n";
  tracing_file << "%       " << (basic ? "Dest" : "End") << "Container string\n";
  tracing_file << "%       Key string\n";
  tracing_file << "%EndEventDef\n";

  // Event
  tracing_file << "%EventDef PajeNewEvent " << PajeEventType::NewEvent << '\n';
  tracing_file << "%       Time date\n";
  tracing_file << "%       Type string\n";
  tracing_file << "%       Container string\n";
  tracing_file << "%       Value string\n";
  tracing_file << "%EndEventDef\n";
}
} // namespace simgrid::instr::paje
