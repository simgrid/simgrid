/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SYNCHRO_IO_HPP
#define _SIMIX_SYNCHRO_IO_HPP

#include "surf/surf.h"
#include "src/kernel/activity/ActivityImpl.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

  XBT_PUBLIC_CLASS Io : public ActivityImpl {
  public:
    void suspend() override;
    void resume() override;
    void post() override;

    sg_host_t host = nullptr;
    surf_action_t surf_io = nullptr;
  };

}}} // namespace simgrid::kernel::activity

#endif
