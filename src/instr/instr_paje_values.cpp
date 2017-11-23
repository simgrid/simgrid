/* Copyright (c) 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");
extern FILE* tracing_file;

namespace simgrid {
namespace instr {

EntityValue::EntityValue(std::string name, std::string color, Type* father)
    : id_(instr_new_paje_id()), name_(name), color_(color), father_(father){};

void EntityValue::print()
{
  if (instr_fmt_type != instr_fmt_paje)
    return;
  std::stringstream stream;
  XBT_DEBUG("%s: event_type=%u", __FUNCTION__, PAJE_DefineEntityValue);
  stream << std::fixed << std::setprecision(TRACE_precision()) << PAJE_DefineEntityValue;
  stream << " " << id_ << " " << father_->getId() << " " << name_;
  if (not color_.empty())
    stream << " \"" << color_ << "\"";
  XBT_DEBUG("Dump %s", stream.str().c_str());
  fprintf(tracing_file, "%s\n", stream.str().c_str());
}

}
}
