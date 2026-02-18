/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MemoryImpl.hpp" // MemoryAccessType

namespace simgrid::mc {

class MemOp {
  MemOpType type_;
  void* location_;
  unsigned char size_;

public:
  MemOp(MemOpType type, void* where, unsigned char size) : type_(type), location_(where), size_(size) {}
  MemOpType get_type() const { return type_; }
  void* get_location() const { return location_; }
  unsigned char get_size() const { return size_; }

  bool is_racy_with(MemOp* other)
  {
    if (location_ != other->location_)
      return false;

    if (type_ == MemOpType::READ && other->type_ == MemOpType::READ)
      return false;

    return true;
  }
};

} // namespace simgrid::mc
