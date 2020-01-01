/* Copyright (c) 2014-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>
#include <cstdint>

#include "xbt/log.h"

#include "mc/datatypes.h"
#include "src/mc/mc_hash.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/sosp/Snapshot.hpp"
#include <mc/mc.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_hash, mc, "Logging specific to mc_hash");

namespace simgrid {
namespace mc {

namespace {

class djb_hash {
  hash_type state_ = 5381LL;

public:
  template<class T>
  void update(T& x)
  {
    state_ = (state_ << 5) + state_ + x;
  }
  hash_type value()
  {
    return state_;
  }
};

}

hash_type hash(Snapshot const& snapshot)
{
  XBT_DEBUG("START hash %i", snapshot.num_state_);
  djb_hash hash;
  // TODO:
  // * nb_processes
  // * heap_bytes_used
  // * root variables
  // * basic stack frame information
  // * stack frame local variables
  XBT_DEBUG("END hash %i", snapshot.num_state_);
  return hash.value();
}

}
}
