/* Copyright (c) 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>

#include "src/instr/instr_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

simgrid::instr::Value::Value(std::string name, std::string color, simgrid::instr::Type* father)
    : name_(name), color_(color), father_(father)
{
  if (name.empty() || father == nullptr) {
    THROWF(tracing_error, 0, "can't create a value with no name (or a nullptr father)");
  }
  this->id_    = std::to_string(instr_new_paje_id());

  father->values_.insert({name, this});
  XBT_DEBUG("new value %s, child of %s", name_.c_str(), father_->name_);
  print();
};

simgrid::instr::Value* simgrid::instr::Value::byNameOrCreate(std::string name, std::string color,
                                                             simgrid::instr::Type* father)
{
  Value* ret = 0;
  try {
    ret = Value::byName(name, father);
  } catch (xbt_ex& e) {
    ret = new Value(name, color, father);
  }
  return ret;
}

simgrid::instr::Value* simgrid::instr::Value::byName(std::string name, Type* father)
{
  if (name.empty() || father == nullptr) {
    THROWF(tracing_error, 0, "can't get a value with no name (or a nullptr father)");
  }

  if (father->kind_ == TYPE_VARIABLE)
    THROWF(tracing_error, 0, "variables can't have different values (%s)", father->name_);
  auto ret = father->values_.find(name);
  if (ret == father->values_.end()) {
    THROWF(tracing_error, 2, "value with name (%s) not found in father type (%s)", name.c_str(), father->name_);
  }
  return ret->second;
}
