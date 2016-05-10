/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SYNCHRO_EXEC_HPP
#define _SIMIX_SYNCHRO_EXEC_HPP

#include "surf/surf.h"
#include "src/simix/Synchro.h"

namespace simgrid {
namespace simix {

  XBT_PUBLIC_CLASS Exec : public Synchro {
    ~Exec();
  public:
    void suspend();
    void resume();
    void post() override;
    double remains();

    sg_host_t host;                /* The host where the execution takes place */
    surf_action_t surf_exec;        /* The Surf execution action encapsulated */
  };

}} // namespace simgrid::simix
#endif
