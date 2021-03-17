/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ExecImpl.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Exec.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"

#include "simgrid/s4u/Host.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

void simcall_HANDLER_execution_waitany_for(smx_simcall_t simcall,
                                           const std::vector<simgrid::kernel::activity::ExecImpl*>* execs,
                                           double timeout)
{
  if (timeout < 0.0) {
    simcall->timeout_cb_ = nullptr;
  } else {
    simcall->timeout_cb_ = simgrid::simix::Timer::set(SIMIX_get_clock() + timeout, [simcall, execs]() {
      simcall->timeout_cb_ = nullptr;
      for (auto* exec : *execs) {
        // Remove the first occurrence of simcall:
        auto j = boost::range::find(exec->simcalls_, simcall);
        if (j != exec->simcalls_.end())
          exec->simcalls_.erase(j);
      }
      simcall_execution_waitany_for__set__result(simcall, -1);
      simcall->issuer_->simcall_answer();
    });
  }

  for (auto* exec : *execs) {
    /* associate this simcall to the the synchro */
    exec->simcalls_.push_back(simcall);

    /* see if the synchro is already finished */
    if (exec->state_ != simgrid::kernel::activity::State::WAITING &&
        exec->state_ != simgrid::kernel::activity::State::RUNNING) {
      exec->finish();
      break;
    }
  }
}

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
    timeout_detector_.reset(hosts_.front()->pimpl_cpu->sleep(timeout));
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
      surf_action_ = hosts_.front()->pimpl_cpu->execution_start(flops_amounts_.front());
      surf_action_->set_sharing_penalty(sharing_penalty_);
      surf_action_->set_category(get_tracing_category());

      if (bound_ > 0) {
        surf_action_->set_bound(bound_);
        surf_action_->set_user_bound(bound_);
      }
    } else {
      // FIXME[donassolo]: verify if all hosts belongs to the same netZone?
      auto host_model = hosts_.front()->get_netpoint()->get_englobing_zone()->get_host_model();
      surf_action_    = host_model->execute_parallel(hosts_, flops_amounts_.data(), bytes_amounts_.data(), -1);
    }
    surf_action_->set_activity(this);
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

  get_iface()->set_finish_time(surf_action_->get_finish_time());

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
  while (not simcalls_.empty()) {
    smx_simcall_t simcall = simcalls_.front();
    simcalls_.pop_front();

    /* If a waitany simcall is waiting for this synchro to finish, then remove it from the other synchros in the waitany
     * list. Afterwards, get the position of the actual synchro in the waitany list and return it as the result of the
     * simcall */

    if (simcall->call_ == simix::Simcall::NONE) // FIXME: maybe a better way to handle this case
      continue;                                 // if process handling comm is killed
    if (simcall->call_ == simix::Simcall::EXECUTION_WAITANY_FOR) {
      const std::vector<simgrid::kernel::activity::ExecImpl*>* execs =
          simcall_execution_waitany_for__get__execs(simcall);

      for (auto* exec : *execs) {
        // Remove the first occurrence of simcall:
        auto j = boost::range::find(exec->simcalls_, simcall);
        if (j != exec->simcalls_.end())
          exec->simcalls_.erase(j);

        if (simcall->timeout_cb_) {
          simcall->timeout_cb_->remove();
          simcall->timeout_cb_ = nullptr;
        }
      }

      if (not MC_is_active() && not MC_record_replay_is_active()) {
        auto element = std::find(execs->begin(), execs->end(), this);
        int rank     = (element != execs->end()) ? std::distance(execs->begin(), element) : -1;
        simcall_execution_waitany_for__set__result(simcall, rank);
      }
    }
    switch (state_) {
      case State::DONE:
        /* do nothing, synchro done */
        XBT_DEBUG("ExecImpl::finish(): execution successful");
        break;

      case State::FAILED:
        XBT_DEBUG("ExecImpl::finish(): host '%s' failed", simcall->issuer_->get_host()->get_cname());
        simcall->issuer_->context_->set_wannadie();
        if (simcall->issuer_->get_host()->is_on())
          simcall->issuer_->exception_ =
              std::make_exception_ptr(simgrid::HostFailureException(XBT_THROW_POINT, "Host failed"));
        /* else, the actor will be killed with no possibility to survive */
        break;

      case State::CANCELED:
        XBT_DEBUG("ExecImpl::finish(): execution canceled");
        simcall->issuer_->exception_ =
            std::make_exception_ptr(simgrid::CancelException(XBT_THROW_POINT, "Execution Canceled"));
        break;

      case State::TIMEOUT:
        XBT_DEBUG("ExecImpl::finish(): execution timeouted");
        simcall->issuer_->exception_ = std::make_exception_ptr(simgrid::TimeoutException(XBT_THROW_POINT, "Timeouted"));
        break;

      default:
        xbt_die("Internal error in ExecImpl::finish(): unexpected synchro state %d", static_cast<int>(state_));
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
    resource::Action* new_action = to->pimpl_cpu->execution_start(old_action->get_cost());
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

/*************
 * Callbacks *
 *************/
xbt::signal<void(ExecImpl const&, s4u::Host*)> ExecImpl::on_migration;

} // namespace activity
} // namespace kernel
} // namespace simgrid
