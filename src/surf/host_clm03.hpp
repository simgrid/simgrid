/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>

#include "StorageImpl.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"
#include "src/surf/HostImpl.hpp"

#ifndef SURF_HOST_CLM03_HPP_
#define SURF_HOST_CLM03_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class XBT_PRIVATE HostCLM03Model;

/*********
 * Model *
 *********/

class HostCLM03Model : public HostModel {
public:
  double nextOccuringEvent(double now) override;
  void updateActionsState(double now, double delta) override;
};
}
}

#endif /* SURF_HOST_CLM03_HPP_ */
