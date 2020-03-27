/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

namespace simgrid {
namespace instr {

EntityValue::EntityValue(const std::string& name, const std::string& color, Type* father)
    : id_(instr_new_paje_id()), name_(name), color_(color), father_(father)
{
  on_creation(*this);
}
} // namespace instr
} // namespace simgrid
