/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_MUTEX_HPP
#define SIMGRID_S4U_MUTEX_HPP

#include <xbt/base.h>
#include "simgrid/simix.h"


namespace simgrid {
namespace s4u {

XBT_PUBLIC_CLASS Mutex {

public:
  Mutex();
  ~Mutex() {};

protected:

public:

  void lock();
  void unlock();
  bool try_lock();

private:
  std::shared_ptr<s_smx_mutex> _mutex;

};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_MUTEX_HPP */
