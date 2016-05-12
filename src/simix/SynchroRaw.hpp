/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SYNCHRO_RAW_HPP
#define _SIMIX_SYNCHRO_RAW_HPP

#include "surf/surf.h"
#include "src/simix/Synchro.h"

namespace simgrid {
namespace simix {

  /** Used to implement mutexes, semaphores and conditions */
  XBT_PUBLIC_CLASS Raw : public Synchro {
  public:
    ~Raw();
    void suspend() override;
    void resume() override;
    void post() override;

    surf_action_t sleep;
  };

}} // namespace simgrid::simix

#endif
