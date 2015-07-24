/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stddef.h>

#include <xbt/dynar.h>

#include "mc_object_info.h"
#include "mc_private.h"
#include "mc/Frame.hpp"
#include "mc/Type.hpp"
#include "mc/Variable.hpp"

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

}
}