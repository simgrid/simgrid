/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ExecImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"

#include "simgrid/s4u/Host.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

void simcall_HANDLER_execution_wait(smx_simcall_t simcall, simgrid::kernel::activity::ExecImpl* synchro)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro, (int)synchro->state_);

  /* Associate this simcall to the synchro */
  synchro->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  /* set surf's synchro */
  if (MC_is_active() || MC_record_replay_is_active()) {
    synchro->state_ = SIMIX_DONE;
    synchro->finish();
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (synchro->state_ != SIMIX_RUNNING)
    synchro->finish();
}

void simcall_HANDLER_execution_test(smx_simcall_t simcall, simgrid::kernel::activity::ExecImpl* synchro)
{
  int res = (synchro->state_ != SIMIX_WAITING && synchro->state_ != SIMIX_RUNNING);
  if (res) {
    synchro->simcalls_.push_back(simcall);
    synchro->finish();
  } else {
    SIMIX_simcall_answer(simcall);
  }
  simcall_execution_test__set__result(simcall, res);
}

namespace simgrid {
namespace kernel {
namespace activity {

ExecImpl::ExecImpl(const std::string& name, const std::string& tracing_category) : ActivityImpl(name)
{
  this->state_ = SIMIX_RUNNING;
  this->set_category(tracing_category);

  XBT_DEBUG("Create exec %p", this);
}

ExecImpl::~ExecImpl()
{
  if (surf_action_)
    surf_action_->unref();
  if (timeout_detector_)
    timeout_detector_->unref();
  XBT_DEBUG("Destroy exec %p", this);
}

ExecImpl* ExecImpl::set_host(s4u::Host* host)
{
  host_ = host;
  return this;
}

ExecImpl* ExecImpl::set_timeout(double timeout)
{
  if (timeout > 0 && not MC_is_active() && not MC_record_replay_is_active()) {
    timeout_detector_ = host_->pimpl_cpu->sleep(timeout);
    timeout_detector_->set_data(this);
  }
  return this;
}

ExecImpl* ExecImpl::start(double flops_amount, double priority, double bound)
{
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    surf_action_ = host_->pimpl_cpu->execution_start(flops_amount);
    surf_action_->set_data(this);
    surf_action_->set_priority(priority);
    if (bound > 0)
      surf_action_->set_bound(bound);
  }

  XBT_DEBUG("Create execute synchro %p: %s", this, get_cname());
  ExecImpl::on_creation(this);
  return this;
}

ExecImpl* ExecImpl::start(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                          const std::vector<double>& bytes_amounts)
{
  /* set surf's synchro */
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    surf_action_ = surf_host_model->execute_parallel(hosts, flops_amounts.data(), bytes_amounts.data(), -1);
    if (surf_action_ != nullptr) {
      surf_action_->set_data(this);
    }
  }
  XBT_DEBUG("Create parallel execute synchro %p", this);
  ExecImpl::on_creation(this);
  return this;
}
void ExecImpl::cancel()
{
  XBT_VERB("This exec %p is canceled", this);
  if (surf_action_ != nullptr)
    surf_action_->cancel();
}

double ExecImpl::get_remaining()
{
  return surf_action_ ? surf_action_->get_remains() : 0;
}

double ExecImpl::get_seq_remaining_ratio()
{
  return (surf_action_ == nullptr) ? 0 : surf_action_->get_remains() / surf_action_->get_cost();
}

double ExecImpl::get_par_remaining_ratio()
{
  // parallel task: their remain is already between 0 and 1
  return (surf_action_ == nullptr) ? 0 : surf_action_->get_remains();
}

void ExecImpl::set_bound(double bound)
{
  if (surf_action_)
    surf_action_->set_bound(bound);
}
void ExecImpl::set_priority(double priority)
{
  if (surf_action_)
    surf_action_->set_priority(priority);
}

void ExecImpl::post()
{
  if (host_ && not host_->is_on()) { /* FIXME: handle resource failure for parallel tasks too */
    /* If the host running the synchro failed, notice it. This way, the asking
     * process can be killed if it runs on that host itself */
    state_ = SIMIX_FAILED;
  } else if (surf_action_ && surf_action_->get_state() == resource::Action::State::FAILED) {
    /* If the host running the synchro didn't fail, then the synchro was canceled */
    state_ = SIMIX_CANCELED;
  } else if (timeout_detector_ && timeout_detector_->get_state() == resource::Action::State::FINISHED) {
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
    finish();
}

void ExecImpl::finish()
{
  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();
    switch (state_) {

      case SIMIX_DONE:
        /* do nothing, synchro done */
        XBT_DEBUG("ExecImpl::finish(): execution successful");
        break;

      case SIMIX_FAILED:
        XBT_DEBUG("ExecImpl::finish(): host '%s' failed", simcall->issuer->get_host()->get_cname());
        simcall->issuer->context_->iwannadie = true;
        if (simcall->issuer->get_host()->is_on())
          simcall->issuer->exception_ =
              std::make_exception_ptr(simgrid::HostFailureException(XBT_THROW_POINT, "Host failed"));
        /* else, the actor will be killed with no possibility to survive */
        break;

      case SIMIX_CANCELED:
        XBT_DEBUG("ExecImpl::finish(): execution canceled");
        simcall->issuer->exception_ =
            std::make_exception_ptr(simgrid::CancelException(XBT_THROW_POINT, "Execution Canceled"));
        break;

      case SIMIX_TIMEOUT:
        XBT_DEBUG("ExecImpl::finish(): execution timeouted");
        simcall->issuer->exception_ = std::make_exception_ptr(simgrid::TimeoutError(XBT_THROW_POINT, "Timeouted"));
        break;

      default:
        xbt_die("Internal error in ExecImpl::finish(): unexpected synchro state %d", static_cast<int>(state_));
    }

    simcall->issuer->waiting_synchro = nullptr;
    simcall_execution_wait__set__result(simcall, state_);

    /* Fail the process if the host is down */
    if (simcall->issuer->get_host()->is_on())
      SIMIX_simcall_answer(simcall);
    else
      simcall->issuer->context_->iwannadie = true;
  }
}

ActivityImpl* ExecImpl::migrate(simgrid::s4u::Host* to)
{
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    resource::Action* old_action = this->surf_action_;
    resource::Action* new_action = to->pimpl_cpu->execution_start(old_action->get_cost());
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
xbt::signal<void(ExecImplPtr)> ExecImpl::on_creation;
xbt::signal<void(ExecImplPtr)> ExecImpl::on_completion;
xbt::signal<void(ExecImplPtr, s4u::Host*)> ExecImpl::on_migration;

} // namespace activity
} // namespace kernel
} // namespace simgrid
