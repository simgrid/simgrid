/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_paje_values, instr, "Paje tracing event system (values)");

val_t PJ_value_new (const char *name, const char *color, type_t father)
{
  if (name == NULL || father == NULL){
    THROWF (tracing_error, 0, "can't create a value with a NULL name (or a NULL father)");
  }

  val_t ret = xbt_new0(s_val_t, 1);
  ret->name = xbt_strdup (name);
  ret->father = father;
  ret->color = xbt_strdup (color);

  char str_id[INSTR_DEFAULT_STR_SIZE];
  snprintf (str_id, INSTR_DEFAULT_STR_SIZE, "%lld", instr_new_paje_id());
  ret->id = xbt_strdup (str_id);

  xbt_dict_set (father->values, name, ret, NULL);
  XBT_DEBUG("new value %s, child of %s", ret->name, ret->father->name);
  new_pajeDefineEntityValue(ret);
  return ret;
}

val_t PJ_value_get_or_new (const char *name, const char *color, type_t father)
{
  val_t ret = 0;
  xbt_ex_t e;
  TRY {
    ret = PJ_value_get(name, father);
  }
  CATCH(e) {
    xbt_ex_free(e);
    ret = PJ_value_new(name, color, father);
  }
  return ret;
}

val_t PJ_value_get (const char *name, type_t father)
{
  if (name == NULL || father == NULL){
    THROWF (tracing_error, 0, "can't get a value with a NULL name (or a NULL father)");
  }

  if (father->kind == TYPE_VARIABLE)
    THROWF(tracing_error, 0, "variables can't have different values (%s)", father->name);
  val_t ret = (val_t)xbt_dict_get_or_null (father->values, name);
  if (ret == NULL) {
    THROWF(tracing_error, 2, "value with name (%s) not found in father type (%s)", name, father->name);
  }
  return ret;
}

void PJ_value_free (val_t value)
{
  XBT_DEBUG("free value %s, child of %s", value->name, value->father->name);
  xbt_free(((val_t)value)->name);
  xbt_free(((val_t)value)->color);
  xbt_free(((val_t)value)->id);
  xbt_free(value);
}
