/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stddef.h>

#include <xbt/dynar.h>

#include "mc_object_info.h"
#include "mc_private.h"

namespace simgrid {
namespace mc {

// Type

Type::Type()
{
  this->type = 0;
  this->id = 0;
  this->byte_size = 0;
  this->element_count = 0;
  this->is_pointer_type = 0;
  this->subtype = nullptr;
  this->full_type = nullptr;
}

// ObjectInformations

dw_frame_t ObjectInformation::find_function(const void *ip) const
{
  xbt_dynar_t dynar = this->functions_index;
  mc_function_index_item_t base =
      (mc_function_index_item_t) xbt_dynar_get_ptr(dynar, 0);
  int i = 0;
  int j = xbt_dynar_length(dynar) - 1;
  while (j >= i) {
    int k = i + ((j - i) / 2);
    if (ip < base[k].low_pc) {
      j = k - 1;
    } else if (ip >= base[k].high_pc) {
      i = k + 1;
    } else {
      return base[k].function;
    }
  }
  return nullptr;
}

dw_variable_t ObjectInformation::find_variable(const char* name) const
{
  unsigned int cursor = 0;
  dw_variable_t variable;
  xbt_dynar_foreach(this->global_variables, cursor, variable){
    if(!strcmp(name, variable->name))
      return variable;
  }

  return nullptr;
}

}
}