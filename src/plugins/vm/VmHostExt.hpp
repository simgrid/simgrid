/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/HostImpl.hpp"

#ifndef VM_HOST_INFO_HPP_
#define VM_HOST_INFO_HPP_

namespace simgrid {
namespace vm {
/** @brief Host extension for the VMs */
class VmHostExt {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, VmHostExt> EXTENSION_ID;
  virtual ~VmHostExt() = default;

  sg_size_t ramsize = 0;    /* available ramsize (0= not taken into account) */
  bool overcommit   = true; /* Whether the host allows overcommiting more VM than the avail ramsize allows */

  static void ensureVmExtInstalled();
};
}
}

#endif /* VM_HOST_INFO_HPP_ */
