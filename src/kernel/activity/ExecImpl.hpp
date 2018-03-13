/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_EXEC_HPP
#define SIMIX_SYNCHRO_EXEC_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC ExecImpl : public ActivityImpl {
  ~ExecImpl() override;

public:
  explicit ExecImpl(const char* name, sg_host_t host);
  void suspend() override;
  void resume() override;
  void post() override;
  double remains();
  double remainingRatio();
  void setBound(double bound);
  virtual ActivityImpl* migrate(s4u::Host* to);

  /* The host where the execution takes place. nullptr means this is a parallel exec (and only surf knows the hosts) */
  sg_host_t host_               = nullptr;
  kernel::resource::Action* surfAction_     = nullptr; /* The Surf execution action encapsulated */
  kernel::resource::Action* timeoutDetector = nullptr;
  static simgrid::xbt::signal<void(kernel::activity::ExecImplPtr)> onCreation;
  static simgrid::xbt::signal<void(kernel::activity::ExecImplPtr)> onCompletion;
  static simgrid::xbt::signal<void(simgrid::kernel::activity::ExecImplPtr, simgrid::s4u::Host*)> onMigration;

};
}
}
} // namespace simgrid::kernel::activity
#endif
