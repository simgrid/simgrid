/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/IoImpl.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/simix/smx_io_private.hpp"

void simgrid::kernel::activity::IoImpl::suspend()
{
  if (surf_action_ != nullptr)
    surf_action_->suspend();
}

void simgrid::kernel::activity::IoImpl::resume()
{
  if (surf_action_ != nullptr)
    surf_action_->resume();
}

void simgrid::kernel::activity::IoImpl::post()
{
  for (smx_simcall_t const& simcall : simcalls_) {
    switch (simcall->call) {
      case SIMCALL_STORAGE_WRITE:
        simcall_storage_write__set__result(simcall, surf_action_->get_cost());
        break;
      case SIMCALL_STORAGE_READ:
        simcall_storage_read__set__result(simcall, surf_action_->get_cost());
        break;
      default:
        break;
    }
  }

  switch (surf_action_->get_state()) {
    case simgrid::kernel::resource::Action::State::FAILED:
      state_ = SIMIX_FAILED;
      break;
    case simgrid::kernel::resource::Action::State::FINISHED:
      state_ = SIMIX_DONE;
      break;
    default:
      THROW_IMPOSSIBLE;
      break;
  }

  SIMIX_io_finish(this);
}
