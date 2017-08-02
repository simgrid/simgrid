/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/ex.hpp>

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

value::value(const char* name, const char* color, type_t father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't create a value with a nullptr name (or a nullptr father)");
  }
<<<<<<< HEAD
  this->ret         = new value[1];
=======
  this->ret         = xbt_new1(value, 1); 
>>>>>>> 6896998a44d0f8dd0f44f5496bfa291dbc5d3751
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

value::~value()
{
// Already destroyed by recursiveDestroyType function.
}

value* value::get_or_new(const char* name, const char* color, type_t father)
{
  value* ret = 0;
  try {
    ret = value::get(name, father);
  }
  catch(xbt_ex& e) {
    ret = new value(name, color, father);
    ret = ret->ret;
  }
  return ret;
}

value* value::get(const char* name, type_t father)
{
  if (name == nullptr || father == nullptr){
    THROWF (tracing_error, 0, "can't get a value with a nullptr name (or a nullptr father)");
  }

  if (father->kind == TYPE_VARIABLE)
    THROWF(tracing_error, 0, "variables can't have different values (%s)", father->name);
  value* ret = (value*)xbt_dict_get_or_null(father->values, name);
  if (ret == nullptr) {
    THROWF(tracing_error, 2, "value with name (%s) not found in father type (%s)", name, father->name);
  }
  return ret;
}
