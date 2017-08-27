/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

Value::Value(const char* name, const char* color, Type* father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't create a value with a nullptr name (or a nullptr father)");
  }
  this->ret         = xbt_new0(Value, 1);
  this->ret->name = xbt_strdup (name);
  this->ret->father = father;
  this->ret->color = xbt_strdup (color);

  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", instr_new_paje_id());
  this->ret->id = xbt_strdup (str_id);

  xbt_dict_set (father->values, name, ret, nullptr);
  XBT_DEBUG("new value %s, child of %s", ret->name, ret->father->name);
  LogEntityValue(this->ret);
};

Value::~Value()
{
  /* FIXME: this should be cleanable
  xbt_free(name);
  xbt_free(color);
  xbt_free(id);
  */
}

Value* Value::get_or_new(const char* name, const char* color, Type* father)
{
  Value* ret = 0;
  try {
    ret = Value::get(name, father);
  }
  catch(xbt_ex& e) {
    Value rett(name, color, father);
    ret = rett.ret;
  }
  return ret;
}

Value* Value::get(const char* name, Type* father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't get a value with a nullptr name (or a nullptr father)");
  }

  if (father->kind == TYPE_VARIABLE)
    THROWF(tracing_error, 0, "variables can't have different values (%s)", father->name);
  Value* ret = (Value*)xbt_dict_get_or_null(father->values, name);
  if (ret == nullptr) {
    THROWF(tracing_error, 2, "value with name (%s) not found in father type (%s)", name, father->name);
  }
  return ret;
}
