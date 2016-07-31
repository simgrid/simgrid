/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SYNCHRO_EXEC_HPP
#define _SIMIX_SYNCHRO_EXEC_HPP

#include "surf/surf.h"
#include "src/kernel/activity/Synchro.h"

namespace simgrid {
namespace kernel {
namespace activity {

  XBT_PUBLIC_CLASS Exec : public Synchro {
    ~Exec() override;
  public:
    Exec(const char*name, sg_host_t host);
    void suspend() override;
    void resume() override;
    void post() override;
    double remains();

    sg_host_t host = nullptr; /* The host where the execution takes place. If nullptr, then this is a parallel exec (and only surf knows the hosts) */
    surf_action_t surf_exec = nullptr; /* The Surf execution action encapsulated */
  };

}}} // namespace simgrid::kernel::activity
#endif
