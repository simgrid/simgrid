/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/IoImpl.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/simix/smx_io_private.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_io);

simgrid::kernel::activity::IoImpl::IoImpl(std::string name, resource::Action* surf_action, s4u::Storage* storage)
    : ActivityImpl(name), storage_(storage), surf_action_(surf_action)
{
  this->state_ = SIMIX_RUNNING;

  surf_action_->set_data(this);

  XBT_DEBUG("Create io %p", this);
}

simgrid::kernel::activity::IoImpl::~IoImpl()
{
  if (surf_action_ != nullptr)
    surf_action_->unref();
  XBT_DEBUG("Destroy io %p", this);
}

void simgrid::kernel::activity::IoImpl::cancel()
{
  XBT_VERB("This exec %p is canceled", this);
  if (surf_action_ != nullptr)
    surf_action_->cancel();
}

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

double simgrid::kernel::activity::IoImpl::get_remaining()
{
  return surf_action_ ? surf_action_->get_remains() : 0;
}

void simgrid::kernel::activity::IoImpl::post()
{
  performed_ioops_ = surf_action_->get_cost();
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
  on_completion(this);

  SIMIX_io_finish(this);
}
/*************
 * Callbacks *
 *************/
simgrid::xbt::signal<void(simgrid::kernel::activity::IoImplPtr)> simgrid::kernel::activity::IoImpl::on_creation;
simgrid::xbt::signal<void(simgrid::kernel::activity::IoImplPtr)> simgrid::kernel::activity::IoImpl::on_completion;
