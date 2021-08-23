/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ExecImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"

#include "simgrid/s4u/Host.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

namespace simgrid {
namespace kernel {
namespace activity {

ExecImpl::ExecImpl()
{
  piface_                = new s4u::Exec(this);
  actor::ActorImpl* self = actor::ActorImpl::self();
  if (self) {
    actor_ = self;
    self->activities_.emplace_back(this);
  }
}

ExecImpl& ExecImpl::set_host(s4u::Host* host)
{
  hosts_.assign(1, host);
  return *this;
}

ExecImpl& ExecImpl::set_hosts(const std::vector<s4u::Host*>& hosts)
{
  hosts_ = hosts;
  return *this;
}

ExecImpl& ExecImpl::set_timeout(double timeout)
{
  if (timeout >= 0 && not MC_is_active() && not MC_record_replay_is_active()) {
    timeout_detector_.reset(hosts_.front()->get_cpu()->sleep(timeout));
    timeout_detector_->set_activity(this);
  }
  return *this;
}

ExecImpl& ExecImpl::set_flops_amount(double flops_amount)
{
  flops_amounts_.assign(1, flops_amount);
  return *this;
}

ExecImpl& ExecImpl::set_flops_amounts(const std::vector<double>& flops_amounts)
{
  flops_amounts_ = flops_amounts;
  return *this;
}

ExecImpl& ExecImpl::set_bytes_amounts(const std::vector<double>& bytes_amounts)
{
  bytes_amounts_ = bytes_amounts;

  return *this;
}

ExecImpl* ExecImpl::start()
{
  state_ = State::RUNNING;
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    if (hosts_.size() == 1) {
      surf_action_ = hosts_.front()->get_cpu()->execution_start(flops_amounts_.front(), bound_);
      surf_action_->set_sharing_penalty(sharing_penalty_);
      surf_action_->set_category(get_tracing_category());
    } else {
      // get the model from first host since we have only 1 by now
      auto host_model = hosts_.front()->get_netpoint()->get_englobing_zone()->get_host_model();
      surf_action_    = host_model->execute_parallel(hosts_, flops_amounts_.data(), bytes_amounts_.data(), -1);
    }
    surf_action_->set_activity(this);
    start_time_ = surf_action_->get_start_time();
  }

  XBT_DEBUG("Create execute synchro %p: %s", this, get_cname());
  return this;
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

ExecImpl& ExecImpl::set_bound(double bound)
{
  bound_ = bound;
  return *this;
}

ExecImpl& ExecImpl::set_sharing_penalty(double sharing_penalty)
{
  sharing_penalty_ = sharing_penalty;
  return *this;
}

void ExecImpl::post()
{
  xbt_assert(surf_action_ != nullptr);
  if (std::any_of(hosts_.begin(), hosts_.end(), [](const s4u::Host* host) { return not host->is_on(); })) {
    /* If one of the hosts running the synchro failed, notice it. This way, the asking
     * process can be killed if it runs on that host itself */
    state_ = State::FAILED;
  } else if (surf_action_->get_state() == resource::Action::State::FAILED) {
    /* If all the hosts are running the synchro didn't fail, then the synchro was canceled */
    state_ = State::CANCELED;
  } else if (timeout_detector_ && timeout_detector_->get_state() == resource::Action::State::FINISHED) {
    if (surf_action_->get_remains() > 0.0) {
      surf_action_->set_state(resource::Action::State::FAILED);
      state_ = State::TIMEOUT;
    } else {
      state_ = State::DONE;
    }
  } else {
    state_ = State::DONE;
  }

  finish_time_ = surf_action_->get_finish_time();

  clean_action();
  timeout_detector_.reset();
  if (actor_) {
    actor_->activities_.remove(this);
    actor_ = nullptr;
  }
  /* Answer all simcalls associated with the synchro */
  finish();
}

void ExecImpl::finish()
{
  XBT_DEBUG("ExecImpl::finish() in state %s", to_c_str(state_));
  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == simix::Simcall::NONE) // FIXME: maybe a better way to handle this case
      continue;                                 // if process handling comm is killed
    if (auto* observer =
            dynamic_cast<kernel::actor::ExecutionWaitanySimcall*>(simcall->observer_)) { // simcall is a wait_any?
      const auto& execs = observer->get_execs();

      for (auto* exec : execs) {
        exec->unregister_simcall(simcall);

        if (simcall->timeout_cb_) {
          simcall->timeout_cb_->remove();
          simcall->timeout_cb_ = nullptr;
        }
      }

      if (not MC_is_active() && not MC_record_replay_is_active()) {
        auto element = std::find(execs.begin(), execs.end(), this);
        int rank     = element != execs.end() ? static_cast<int>(std::distance(execs.begin(), element)) : -1;
        observer->set_result(rank);
      }
    }
    switch (state_) {
      case State::FAILED:
        if (simcall->issuer_->get_host()->is_on())
          simcall->issuer_->exception_ = std::make_exception_ptr(HostFailureException(XBT_THROW_POINT, "Host failed"));
        else /* else, the actor will be killed with no possibility to survive */
          simcall->issuer_->context_->set_wannadie();
        break;

      case State::CANCELED:
        simcall->issuer_->exception_ = std::make_exception_ptr(CancelException(XBT_THROW_POINT, "Execution Canceled"));
        break;

      case State::TIMEOUT:
        simcall->issuer_->exception_ = std::make_exception_ptr(TimeoutException(XBT_THROW_POINT, "Timeouted"));
        break;

      default:
        xbt_assert(state_ == State::DONE, "Internal error in ExecImpl::finish(): unexpected synchro state %s",
                   to_c_str(state_));
    }

    simcall->issuer_->waiting_synchro_ = nullptr;
    /* Fail the process if the host is down */
    if (simcall->issuer_->get_host()->is_on())
      simcall->issuer_->simcall_answer();
    else
      simcall->issuer_->context_->set_wannadie();
  }
}

ActivityImpl* ExecImpl::migrate(s4u::Host* to)
{
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    resource::Action* old_action = this->surf_action_;
    resource::Action* new_action = to->get_cpu()->execution_start(old_action->get_cost(), old_action->get_user_bound());
    new_action->set_remains(old_action->get_remains());
    new_action->set_activity(this);
    new_action->set_sharing_penalty(old_action->get_sharing_penalty());
    new_action->set_user_bound(old_action->get_user_bound());

    old_action->set_activity(nullptr);
    old_action->cancel();
    old_action->unref();
    this->surf_action_ = new_action;
  }

  on_migration(*this, to);
  return this;
}

void ExecImpl::wait_any_for(actor::ActorImpl* issuer, const std::vector<ExecImpl*>& execs, double timeout)
{
  if (timeout < 0.0) {
    issuer->simcall_.timeout_cb_ = nullptr;
  } else {
    issuer->simcall_.timeout_cb_ = timer::Timer::set(s4u::Engine::get_clock() + timeout, [issuer, &execs]() {
      issuer->simcall_.timeout_cb_ = nullptr;
      for (auto* exec : execs)
        exec->unregister_simcall(&issuer->simcall_);
      // default result (-1) is set in actor::ExecutionWaitanySimcall
      issuer->simcall_answer();
    });
  }

  for (auto* exec : execs) {
    /* associate this simcall to the the synchro */
    exec->simcalls_.push_back(&issuer->simcall_);

    /* see if the synchro is already finished */
    if (exec->state_ != State::WAITING && exec->state_ != State::RUNNING) {
      exec->finish();
      break;
    }
  }
}

/*************
 * Callbacks *
 *************/
xbt::signal<void(ExecImpl const&, s4u::Host*)> ExecImpl::on_migration;

} // namespace activity
} // namespace kernel
} // namespace simgrid
