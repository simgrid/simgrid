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
  this->start = nullptr;
  this->end = nullptr;
  this->start_exec = nullptr;
  this->end_exec = nullptr;
  this->start_rw = nullptr;
  this->end_rw = nullptr;
  this->start_ro = nullptr;
  this->end_ro = nullptr;
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

/* Find a function by instruction pointer */
mc_frame_t ObjectInformation::find_function(const void *ip) const
{
  /* This is implemented by binary search on a sorted array.
   *
   * We do quite a lot ot those so we want this to be cache efficient.
   * We pack the only information we need in the index entries in order
   * to successfully do the binary search. We do not need the high_pc
   * during the binary search (only at the end) so it is not included
   * in the index entry. We could use parallel arrays as well.
   *
   * We cannot really use the std:: alogrithm for this.
   * We could use std::binary_search by including the high_pc inside
   * the FunctionIndexEntry.
   */
  const simgrid::mc::FunctionIndexEntry* base =
    this->functions_index.data();
  int i = 0;
  int j = this->functions_index.size() - 1;
  while (j >= i) {
    int k = i + ((j - i) / 2);
    if (ip < base[k].low_pc)
      j = k - 1;
    else if (k <= j && ip >= base[k + 1].low_pc)
      i = k + 1;
    else if (ip < base[k].function->high_pc)
      return base[k].function;
    else
      return nullptr;
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