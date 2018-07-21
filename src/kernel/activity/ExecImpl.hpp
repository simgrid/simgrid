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
  explicit ExecImpl(std::string name, resource::Action* surf_action, resource::Action* timeout_detector,
                    s4u::Host* host);
  void suspend() override;
  void resume() override;
  void cancel();
  void post() override;
  double get_remaining();
  double get_remaining_ratio();
  void set_bound(double bound);
  void set_priority(double priority);
  void set_category(std::string category);
  virtual ActivityImpl* migrate(s4u::Host* to);

  /* The host where the execution takes place. nullptr means this is a parallel exec (and only surf knows the hosts) */
  s4u::Host* host_ = nullptr;
  resource::Action* surf_action_; /* The Surf execution action encapsulated */
private:
  resource::Action* timeout_detector_ = nullptr;

public:
  static simgrid::xbt::signal<void(kernel::activity::ExecImplPtr)> on_creation;
  static simgrid::xbt::signal<void(kernel::activity::ExecImplPtr)> on_completion;
  static simgrid::xbt::signal<void(simgrid::kernel::activity::ExecImplPtr, simgrid::s4u::Host*)> on_migration;
};
}
}
} // namespace simgrid::kernel::activity
#endif
