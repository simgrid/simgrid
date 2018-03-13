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
public:
  void suspend() override;
  void resume() override;
  void post() override;

  simgrid::kernel::resource::Action* surf_io = nullptr;
  };

}}} // namespace simgrid::kernel::activity

#endif
