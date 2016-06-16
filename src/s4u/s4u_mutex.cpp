/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_network_private.h"

#include "simgrid/s4u/mutex.hpp"

namespace simgrid {
namespace s4u {

void Mutex::lock() {
  simcall_mutex_lock(mutex_);
}

void Mutex::unlock() {
  simcall_mutex_unlock(mutex_);
}

bool Mutex::try_lock() {
  return simcall_mutex_trylock(mutex_);
}

}
}
