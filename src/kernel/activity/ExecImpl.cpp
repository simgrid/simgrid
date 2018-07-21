/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.hpp"

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/simix/smx_host_private.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/surf/cpu_interface.hpp"

#include "simgrid/s4u/Host.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

simgrid::kernel::activity::ExecImpl::ExecImpl(std::string name, resource::Action* surf_action,
                                              resource::Action* timeout_detector, s4u::Host* host)
    : ActivityImpl(name), host_(host), surf_action_(surf_action), timeout_detector_(timeout_detector)
{
  this->state_ = SIMIX_RUNNING;

  surf_action_->set_data(this);
  if (timeout_detector != nullptr)
    timeout_detector->set_data(this);

  XBT_DEBUG("Create exec %p", this);
}

simgrid::kernel::activity::ExecImpl::~ExecImpl()
{
  if (surf_action_)
    surf_action_->unref();
  if (timeout_detector_)
    timeout_detector_->unref();
  XBT_DEBUG("Destroy exec %p", this);
}

void simgrid::kernel::activity::ExecImpl::suspend()
{
  XBT_VERB("This exec is suspended (remain: %f)", surf_action_->get_remains());
  if (surf_action_ != nullptr)
    surf_action_->suspend();
}

void simgrid::kernel::activity::ExecImpl::resume()
{
  XBT_VERB("This exec is resumed (remain: %f)", surf_action_->get_remains());
  if (surf_action_ != nullptr)
    surf_action_->resume();
}
void simgrid::kernel::activity::ExecImpl::cancel()
{
  XBT_VERB("This exec %p is canceled", this);
  if (surf_action_ != nullptr)
    surf_action_->cancel();
}

double simgrid::kernel::activity::ExecImpl::get_remaining()
{
  xbt_assert(host_ != nullptr, "Calling remains() on a parallel execution is not allowed. "
                               "We would need to return a vector instead of a scalar. "
                               "Did you mean remainingRatio() instead?");

  return surf_action_ ? surf_action_->get_remains() : 0;
}

double simgrid::kernel::activity::ExecImpl::get_remaining_ratio()
{
  if (host_ ==
      nullptr) // parallel task: their remain is already between 0 and 1 (see comment in ExecImpl::get_remaining())
    return (surf_action_ == nullptr) ? 0 : surf_action_->get_remains();
  else // Actually compute the ratio for sequential tasks
    return (surf_action_ == nullptr) ? 0 : surf_action_->get_remains() / surf_action_->get_cost();
}

void simgrid::kernel::activity::ExecImpl::set_bound(double bound)
{
  if (surf_action_)
    surf_action_->set_bound(bound);
}
void simgrid::kernel::activity::ExecImpl::set_priority(double priority)
{
  if (surf_action_)
    surf_action_->set_priority(priority);
}

void simgrid::kernel::activity::ExecImpl::set_category(std::string category)
{
  if (surf_action_)
    surf_action_->set_category(category);
}

void simgrid::kernel::activity::ExecImpl::post()
{
  if (host_ && host_->is_off()) { /* FIXME: handle resource failure for parallel tasks too */
    /* If the host running the synchro failed, notice it. This way, the asking
     * process can be killed if it runs on that host itself */
    state_ = SIMIX_FAILED;
  } else if (surf_action_ && surf_action_->get_state() == simgrid::kernel::resource::Action::State::FAILED) {
    /* If the host running the synchro didn't fail, then the synchro was canceled */
    state_ = SIMIX_CANCELED;
  } else if (timeout_detector_ &&
             timeout_detector_->get_state() == simgrid::kernel::resource::Action::State::FINISHED) {
    state_ = SIMIX_TIMEOUT;
  } else {
    state_ = SIMIX_DONE;
  }

  on_completion(this);

  if (surf_action_) {
    surf_action_->unref();
    surf_action_ = nullptr;
  }
  if (timeout_detector_) {
    timeout_detector_->unref();
    timeout_detector_ = nullptr;
  }

  /* If there are simcalls associated with the synchro, then answer them */
  if (not simcalls_.empty())
    SIMIX_execution_finish(this);
}

simgrid::kernel::activity::ActivityImpl*
simgrid::kernel::activity::ExecImpl::migrate(simgrid::s4u::Host* to)
{

  if (not MC_is_active() && not MC_record_replay_is_active()) {
    simgrid::kernel::resource::Action* old_action = this->surf_action_;
    simgrid::kernel::resource::Action* new_action = to->pimpl_cpu->execution_start(old_action->get_cost());
    new_action->set_remains(old_action->get_remains());
    new_action->set_data(this);
    new_action->set_priority(old_action->get_priority());

    // FIXME: the user-defined bound seem to not be kept by LMM, that seem to overwrite it for the multi-core modeling.
    // I hope that the user did not provide any.

    old_action->set_data(nullptr);
    old_action->cancel();
    old_action->unref();
    this->surf_action_ = new_action;
  }

  on_migration(this, to);
  return this;
}

/*************
 * Callbacks *
 *************/
simgrid::xbt::signal<void(simgrid::kernel::activity::ExecImplPtr)> simgrid::kernel::activity::ExecImpl::on_creation;
simgrid::xbt::signal<void(simgrid::kernel::activity::ExecImplPtr)> simgrid::kernel::activity::ExecImpl::on_completion;
simgrid::xbt::signal<void(simgrid::kernel::activity::ExecImplPtr, simgrid::s4u::Host*)>
    simgrid::kernel::activity::ExecImpl::on_migration;
