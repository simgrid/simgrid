/* Copyright (c) 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>
#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

namespace simgrid {
namespace instr {

Value::Value(std::string name, std::string color, Type* father) : name_(name), color_(color), father_(father)
{
  this->id_    = std::to_string(instr_new_paje_id());
};

Value::~Value()
{
  XBT_DEBUG("free value %s, child of %s", getCname(), father_->getCname());
}

}
}
