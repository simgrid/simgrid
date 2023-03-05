/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Engine.hpp>

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/HostImpl.hpp"
#include "src/mc/mc_replay.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_cpu, kernel, "Kernel cpu-related synchronization");

namespace simgrid::kernel::activity {

ExecImpl::ExecImpl()
{
  piface_                = new s4u::Exec(this);
  actor::ActorImpl* self = actor::ActorImpl::self();
  if (self) {
    set_actor(self);
    self->activities_.insert(this);
  }
}

ExecImpl& ExecImpl::set_host(s4u::Host* host)
{
  ActivityImpl::set_hosts({host});
  return *this;
}

ExecImpl& ExecImpl::set_hosts(const std::vector<s4u::Host*>& hosts)
{
  ActivityImpl::set_hosts(hosts);
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
ExecImpl& ExecImpl::set_thread_count(int thread_count)
{
  thread_count_ = thread_count;

  return *this;
}

ExecImpl* ExecImpl::start()
{
  set_state(State::RUNNING);
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    if (get_hosts().size() == 1) {
      xbt_assert(not flops_amounts_.empty(), "Cannot start Exec: no flops_amount defined.");
      if (thread_count_ == 1) {
        model_action_ = get_host()->get_cpu()->execution_start(flops_amounts_.front(), bound_);
        model_action_->set_sharing_penalty(sharing_penalty_);
      } else {
        auto host_model = get_host()->get_netpoint()->get_englobing_zone()->get_host_model();
        model_action_   = host_model->execute_thread(get_host(), flops_amounts_.front(), thread_count_);
      }
      model_action_->set_category(get_tracing_category());
    } else {
      // get the model from first host since we have only 1 by now
      auto host_model = get_host()->get_netpoint()->get_englobing_zone()->get_host_model();
      model_action_   = host_model->execute_parallel(get_hosts(), flops_amounts_.data(), bytes_amounts_.data(), -1);
    }
    model_action_->set_activity(this);
    set_start_time(model_action_->get_start_time());
  }

  XBT_DEBUG("Create execute synchro %p: %s", this, get_cname());
  return this;
}

double ExecImpl::get_remaining() const
{
  if (get_state() == State::WAITING || get_state() == State::FAILED)
    return flops_amounts_.front();
  return ActivityImpl::get_remaining();
}

double ExecImpl::get_seq_remaining_ratio()
{
  if (get_state() == State::WAITING)
    return 1;
  return (model_action_ == nullptr) ? 0 : model_action_->get_remains() / model_action_->get_cost();
}

double ExecImpl::get_par_remaining_ratio()
{
  // parallel task: their remain is already between 0 and 1
  if (get_state() == State::WAITING)
    return 1;
  return (model_action_ == nullptr) ? 0 : model_action_->get_remains();
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

ExecImpl& ExecImpl::update_sharing_penalty(double sharing_penalty)
{
  sharing_penalty_ = sharing_penalty;
  model_action_->set_sharing_penalty(sharing_penalty);
  return *this;
}

void ExecImpl::set_exception(actor::ActorImpl* issuer)
{
  switch (get_state()) {
    case State::FAILED:
      static_cast<s4u::Exec*>(get_iface())->complete(s4u::Activity::State::FAILED);
      if (issuer->get_host()->is_on())
        issuer->exception_ = std::make_exception_ptr(HostFailureException(XBT_THROW_POINT, "Host failed"));
      else /* else, the actor will be killed with no possibility to survive */
        issuer->set_wannadie();
      break;

    case State::CANCELED:
      issuer->exception_ = std::make_exception_ptr(CancelException(XBT_THROW_POINT, "Execution Canceled"));
      break;

    case State::TIMEOUT:
      issuer->exception_ = std::make_exception_ptr(TimeoutException(XBT_THROW_POINT, "Timeouted"));
      break;

    default:
      xbt_assert(get_state() == State::DONE, "Internal error in ExecImpl::finish(): unexpected synchro state %s",
                 get_state_str());
  }
}
void ExecImpl::finish()
{
  XBT_DEBUG("ExecImpl::finish() in state %s", get_state_str());
  if (model_action_ != nullptr) {
    if (auto const& hosts = get_hosts();
        std::any_of(hosts.begin(), hosts.end(), [](const s4u::Host* host) { return not host->is_on(); })) {
      /* If one of the hosts running the synchro failed, notice it. This way, the asking
       * process can be killed if it runs on that host itself */
      set_state(State::FAILED);
    } else if (model_action_->get_state() == resource::Action::State::FAILED) {
      /* If all the hosts are running the synchro didn't fail, then the synchro was canceled */
      set_state(State::CANCELED);
    } else {
      set_state(State::DONE);
    }

    clean_action();
  }

  if (get_actor() != nullptr)
    get_actor()->activities_.erase(this);

  while (not simcalls_.empty()) {
    actor::Simcall* simcall = simcalls_.front();
    simcalls_.pop_front();

    if (simcall->call_ == actor::Simcall::Type::NONE) // FIXME: maybe a better way to handle this case
      continue;                                       // if process handling comm is killed

    handle_activity_waitany(simcall);

    set_exception(simcall->issuer_);

    simcall->issuer_->waiting_synchro_ = nullptr;
    /* Fail the process if the host is down */
    if (simcall->issuer_->get_host()->is_on())
      simcall->issuer_->simcall_answer();
    else
      simcall->issuer_->set_wannadie();
  }
}

void ExecImpl::reset()
{
  clear_hosts();
  set_start_time(-1.0);
}

ActivityImpl* ExecImpl::migrate(s4u::Host* to)
{
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    resource::Action* old_action = this->model_action_;
    resource::Action* new_action = to->get_cpu()->execution_start(old_action->get_cost(), old_action->get_user_bound());
    new_action->set_remains(old_action->get_remains());
    new_action->set_activity(this);
    new_action->set_sharing_penalty(old_action->get_sharing_penalty());
    new_action->set_user_bound(old_action->get_user_bound());

    old_action->set_activity(nullptr);
    old_action->cancel();
    old_action->unref();
    this->model_action_ = new_action;
  }

  on_migration(*this, to);
  return this;
}

/*************
 * Callbacks *
 *************/
xbt::signal<void(ExecImpl const&, s4u::Host*)> ExecImpl::on_migration;

} // namespace simgrid::kernel::activity
