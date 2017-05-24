/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SRC_KERNEL_ACTIVITY_COMMIMPL_HPP_
#define SRC_KERNEL_ACTIVITY_COMMIMPL_HPP_

#include "simgrid/forward.h"

#include <atomic>
#include <simgrid/simix.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

XBT_PUBLIC_CLASS CommImpl : ActivityImpl
{
public:
  CommImpl() : piface_(this){};
  ~CommImpl() = default;

  using Ptr = boost::intrusive_ptr<ActivityImpl>;
  simgrid::s4u::Comm piface_; // Our interface
};
}
}
}

#endif /* SRC_KERNEL_ACTIVITY_COMMIMPL_HPP_ */
