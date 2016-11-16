/* Copyright (c) 2004-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "src/surf/HostImpl.hpp"

#ifndef VM_HOST_INFO_HPP_
#define VM_HOST_INFO_HPP_

namespace simgrid {
namespace vm {
/** @brief Host extension for the VMs */
class VmHostExt {
  virtual ~VmHostExt();
  static simgrid::xbt::Extension<simgrid::s4u::Host, VmHostExt> EXTENSION_ID;
};
}
}

#endif /* VM_HOST_INFO_HPP_ */
