/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_IO_HPP
#define SIMIX_SYNCHRO_IO_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

XBT_PUBLIC_CLASS IoImpl : public ActivityImpl
{
public:
  void suspend() override;
  void resume() override;
  void post() override;

  surf_action_t surf_io = nullptr;
  };

}}} // namespace simgrid::kernel::activity

#endif
