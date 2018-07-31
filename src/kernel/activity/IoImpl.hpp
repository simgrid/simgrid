/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_IO_HPP
#define SIMIX_SYNCHRO_IO_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC IoImpl : public ActivityImpl {
  ~IoImpl() override;

public:
  explicit IoImpl(std::string name, resource::Action* surf_action, s4u::Storage* storage);

public:
  void suspend() override;
  void resume() override;
  void post() override;
  void cancel();
  double get_remaining();

  s4u::Storage* storage_                          = nullptr;
  simgrid::kernel::resource::Action* surf_action_ = nullptr;

  static simgrid::xbt::signal<void(kernel::activity::IoImplPtr)> on_creation;
  static simgrid::xbt::signal<void(kernel::activity::IoImplPtr)> on_completion;
};
}
}
} // namespace simgrid::kernel::activity

#endif
