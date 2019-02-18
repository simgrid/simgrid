/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "src/kernel/activity/IoImpl.hpp"
#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/StorageImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix, "Logging specific to SIMIX (io)");

void simcall_HANDLER_io_wait(smx_simcall_t simcall, smx_activity_t synchro)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro.get(), (int)synchro->state_);

  /* Associate this simcall to the synchro */
  synchro->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  /* set surf's synchro */
  if (MC_is_active() || MC_record_replay_is_active()) {
    synchro->state_ = SIMIX_DONE;
    boost::static_pointer_cast<simgrid::kernel::activity::IoImpl>(synchro)->finish();
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (synchro->state_ != SIMIX_RUNNING)
    boost::static_pointer_cast<simgrid::kernel::activity::IoImpl>(synchro)->finish();
}

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
  state_ = SIMIX_CANCELED;
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

  finish();
}

void IoImpl::finish()
{
  for (smx_simcall_t const& simcall : simcalls_) {
    switch (state_) {
      case SIMIX_DONE:
        /* do nothing, synchro done */
        break;
      case SIMIX_FAILED:
        simcall->issuer->exception_ =
            std::make_exception_ptr(simgrid::StorageFailureException(XBT_THROW_POINT, "Storage failed"));
        break;
      case SIMIX_CANCELED:
        simcall->issuer->exception_ =
            std::make_exception_ptr(simgrid::CancelException(XBT_THROW_POINT, "I/O Canceled"));
        break;
      default:
        xbt_die("Internal error in SIMIX_io_finish: unexpected synchro state %d", static_cast<int>(state_));
    }

    simcall->issuer->waiting_synchro = nullptr;
    if (simcall->issuer->get_host()->is_on())
      SIMIX_simcall_answer(simcall);
    else
      simcall->issuer->context_->iwannadie = true;
  }
}

/*************
 * Callbacks *
 *************/
xbt::signal<void(IoImplPtr)> IoImpl::on_start;
xbt::signal<void(IoImplPtr)> IoImpl::on_completion;

} // namespace activity
} // namespace kernel
} // namespace simgrid
