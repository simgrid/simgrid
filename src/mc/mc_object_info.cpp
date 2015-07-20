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

static void mc_frame_free(void* frame)
{
  delete (simgrid::mc::Frame*)frame;
}

static void mc_type_free(void* t)
{
  delete (simgrid::mc::Type*)t;
}

// Type

Type::Type()
{
  this->type = 0;
  this->id = 0;
  this->byte_size = 0;
  this->element_count = 0;
  this->is_pointer_type = 0;
  this->type_id = 0;
  this->subtype = nullptr;
  this->full_type = nullptr;
}

// Type

Variable::Variable()
{
  this->dwarf_offset = 0;
  this->global = 0;
  this->type = nullptr;
  this->type_id = 0;
  this->address = nullptr;
  this->start_scope = 0;
  this->object_info = nullptr;
}

// Frame

Frame::Frame()
{
  this->tag = 0;
  this->low_pc = nullptr;
  this->high_pc = nullptr;
  this->id = 0;
  this->abstract_origin_id = 0;
  this->object_info = nullptr;
}

// ObjectInformations

ObjectInformation::ObjectInformation()
{
  this->flags = 0;
  this->file_name = nullptr;
  this->start = nullptr;
  this->end = nullptr;
  this->start_exec = nullptr;
  this->end_exec = nullptr;
  this->start_rw = nullptr;
  this->end_rw = nullptr;
  this->start_ro = nullptr;
  this->end_ro = nullptr;
  this->functions_index = nullptr;
}

ObjectInformation::~ObjectInformation()
{
  xbt_free(this->file_name);
  xbt_dynar_free(&this->functions_index);
}

/** Find the DWARF offset for this ELF object
 *
 *  An offset is applied to address found in DWARF:
 *
 *  <ul>
 *    <li>for an executable obejct, addresses are virtual address
 *        (there is no offset) i.e. \f$\text{virtual address} = \{dwarf address}\f$;</li>
 *    <li>for a shared object, the addreses are offset from the begining
 *        of the shared object (the base address of the mapped shared
 *        object must be used as offset
 *        i.e. \f$\text{virtual address} = \text{shared object base address}
 *             + \text{dwarf address}\f$.</li>
 *
 */
void *ObjectInformation::base_address() const
{
  if (this->executable())
    return nullptr;

  void *result = this->start_exec;
  if (this->start_rw != NULL && result > (void *) this->start_rw)
    result = this->start_rw;
  if (this->start_ro != NULL && result > (void *) this->start_ro)
    result = this->start_ro;
  return result;
}

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

simgrid::mc::Variable* ObjectInformation::find_variable(const char* name) const
{
  for (simgrid::mc::Variable& variable : this->global_variables)
    if(variable.name == name)
      return &variable;
  return nullptr;
}

}
}