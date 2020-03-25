/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");
extern std::ofstream tracing_file;

namespace simgrid {
namespace instr {

EntityValue::EntityValue(const std::string& name, const std::string& color, Type* father)
    : id_(instr_new_paje_id()), name_(name), color_(color), father_(father){}

void EntityValue::print()
{
  if (trace_format != simgrid::instr::TraceFormat::Paje)
    return;
  std::stringstream stream;
  XBT_DEBUG("%s: event_type=%u", __func__, PAJE_DefineEntityValue);
  stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_DefineEntityValue;
  stream << " " << id_ << " " << father_->get_id() << " " << name_;
  if (not color_.empty())
    stream << " \"" << color_ << "\"";
  XBT_DEBUG("Dump %s", stream.str().c_str());
  tracing_file << stream.str() << std::endl;
}

} // namespace instr
} // namespace simgrid
