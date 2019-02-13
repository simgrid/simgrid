/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/IoImpl.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/simix/smx_io_private.hpp"
#include "src/surf/StorageImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_io);
namespace simgrid {
namespace kernel {
namespace activity {

IoImpl::IoImpl(std::string name, surf::StorageImpl* storage) : ActivityImpl(std::move(name)), storage_(storage)
{
  this->state_ = SIMIX_RUNNING;

  XBT_DEBUG("Create io impl %p", this);
}

IoImpl::~IoImpl()
{
  if (surf_action_ != nullptr)
    surf_action_->unref();
  XBT_DEBUG("Destroy io %p", this);
}

IoImpl* IoImpl::start(sg_size_t size, simgrid::s4u::Io::OpType type)
{
  surf_action_ = storage_->io_start(size, type);
  surf_action_->set_data(this);

  XBT_DEBUG("Create IO synchro %p %s", this, get_cname());
  simgrid::kernel::activity::IoImpl::on_start(this);

  return this;
}

void IoImpl::cancel()
{
  XBT_VERB("This exec %p is canceled", this);
  if (surf_action_ != nullptr)
    surf_action_->cancel();
}

double IoImpl::get_remaining()
{
  return surf_action_ ? surf_action_->get_remains() : 0;
}

void IoImpl::post()
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
xbt::signal<void(IoImplPtr)> IoImpl::on_start;
xbt::signal<void(IoImplPtr)> IoImpl::on_completion;

} // namespace activity
} // namespace kernel
} // namespace simgrid
