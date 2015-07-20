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

// Free functions

static void mc_variable_free_voidp(void *t)
{
  delete *(simgrid::mc::Variable**)t;
}

static void mc_frame_free_voipd(void** p)
{
  delete *(mc_frame_t**)p;
  *p = nullptr;
}

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

// Type

Variable::Variable()
{
  this->dwarf_offset = 0;
  this->global = 0;
  this->type = nullptr;
  this->location_list = {0, nullptr};
  this->address = nullptr;
  this->start_scope = 0;
  this->object_info = nullptr;
}

Variable::~Variable()
{
  if (this->location_list.locations)
    mc_dwarf_location_list_clear(&this->location_list);
}

// Frame

Frame::Frame()
{
  this->tag = 0;
  this->low_pc = nullptr;
  this->high_pc = nullptr;
  this->frame_base = {0, nullptr};
  this->variables = xbt_dynar_new(
    sizeof(mc_variable_t), mc_variable_free_voidp);
  this->id = 0;
  this->scopes = xbt_dynar_new(
    sizeof(mc_frame_t), (void_f_pvoid_t) mc_frame_free_voipd);
  this->abstract_origin_id = 0;
  this->object_info = nullptr;
}

Frame::~Frame()
{
  mc_dwarf_location_list_clear(&(this->frame_base));
  xbt_dynar_free(&(this->variables));
  xbt_dynar_free(&(this->scopes));
}

// ObjectInformations

mc_frame_t ObjectInformation::find_function(const void *ip) const
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

mc_variable_t ObjectInformation::find_variable(const char* name) const
{
  unsigned int cursor = 0;
  mc_variable_t variable;
  xbt_dynar_foreach(this->global_variables, cursor, variable)
    if(variable->name == name)
      return variable;
  return nullptr;
}

}
}