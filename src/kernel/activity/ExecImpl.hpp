/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_EXEC_HPP
#define SIMIX_SYNCHRO_EXEC_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.h"

namespace simgrid {
namespace kernel {
namespace activity {

XBT_PUBLIC_CLASS ExecImpl : public ActivityImpl
{
  ~ExecImpl() override;

public:
  ExecImpl(const char* name, sg_host_t host);
  void suspend() override;
  void resume() override;
  void post() override;
  double remains();

  sg_host_t host_ =
      nullptr; /* The host where the execution takes place. If nullptr, then this is a parallel exec (and only surf
                  knows the hosts) */
  surf_action_t surf_exec       = nullptr; /* The Surf execution action encapsulated */
  surf::Action* timeoutDetector = nullptr;
};
}
}
} // namespace simgrid::kernel::activity
#endif
