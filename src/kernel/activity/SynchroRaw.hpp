/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_RAW_HPP
#define SIMIX_SYNCHRO_RAW_HPP

#include "surf/surf.h"
#include "src/kernel/activity/ActivityImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

  /** Used to implement mutexes, semaphores and conditions */
  XBT_PUBLIC_CLASS Raw : public ActivityImpl {
  public:
    ~Raw() override;
    void suspend() override;
    void resume() override;
    void post() override;

    surf_action_t sleep = nullptr;
  };

}}} // namespace simgrid::kernel::activity

#endif
