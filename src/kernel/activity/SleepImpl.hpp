/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_SLEEP_HPP
#define SIMIX_SYNCHRO_SLEEP_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

XBT_PUBLIC_CLASS SleepImpl : public ActivityImpl
{
public:
  void suspend() override;
  void resume() override;
  void post() override;

  sg_host_t host           = nullptr; /* The host that is sleeping */
  surf_action_t surf_sleep = nullptr; /* The Surf sleeping action encapsulated */
};
}
}
} // namespace simgrid::kernel::activity

#endif
