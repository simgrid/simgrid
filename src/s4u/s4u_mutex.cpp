/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_network_private.h"

#include "simgrid/s4u/mutex.hpp"


using namespace simgrid;

s4u::Mutex::Mutex() {
    smx_mutex_t smx_mutex = simcall_mutex_init();
    _mutex = std::shared_ptr<simgrid::simix::Mutex>(smx_mutex, SIMIX_mutex_destroy  );
}

void s4u::Mutex::lock() {
  simcall_mutex_lock(_mutex.get());
}

void s4u::Mutex::unlock() {
  simcall_mutex_unlock(_mutex.get());
}

bool s4u::Mutex::try_lock() {
  return simcall_mutex_trylock(_mutex.get());
}
