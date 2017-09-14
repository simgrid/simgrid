/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

simgrid::instr::Value::Value(const char* name, const char* color, simgrid::instr::Type* father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't create a value with a nullptr name (or a nullptr father)");
  }
  this->name_   = xbt_strdup(name);
  this->father_ = father;
  this->color_  = xbt_strdup(color);

  this->id_ = bprintf("%lld", instr_new_paje_id());

  xbt_dict_set(father->values_, name, this, nullptr);
  XBT_DEBUG("new value %s, child of %s", name_, father_->name_);
  LogEntityValue(this);
};

simgrid::instr::Value::~Value()
{
  /* FIXME: this should be cleanable
  xbt_free(name);
  xbt_free(color);
  xbt_free(id);
  */
}

simgrid::instr::Value* simgrid::instr::Value::get_or_new(const char* name, const char* color,
                                                         simgrid::instr::Type* father)
{
  Value* ret = 0;
  try {
    ret = Value::get(name, father);
  }
  catch(xbt_ex& e) {
    ret = new Value(name, color, father);
  }
  return ret;
}

simgrid::instr::Value* simgrid::instr::Value::get(const char* name, Type* father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't get a value with a nullptr name (or a nullptr father)");
  }

  if (father->kind_ == TYPE_VARIABLE)
    THROWF(tracing_error, 0, "variables can't have different values (%s)", father->name_);
  Value* ret = (Value*)xbt_dict_get_or_null(father->values_, name);
  if (ret == nullptr) {
    THROWF(tracing_error, 2, "value with name (%s) not found in father type (%s)", name, father->name_);
  }
  return ret;
}
