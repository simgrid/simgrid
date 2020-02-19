/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/exec.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_exec, s4u_activity, "S4U asynchronous executions");

namespace simgrid {
namespace s4u {
xbt::signal<void(Actor const&, Exec const&)> Exec::on_start;
xbt::signal<void(Actor const&, Exec const&)> Exec::on_completion;

Exec::Exec()
{
  pimpl_ = kernel::activity::ExecImplPtr(new kernel::activity::ExecImpl());
}

Exec* Exec::wait()
{
  return this->wait_for(-1);
}

Exec* Exec::wait_for(double timeout)
{
  if (state_ == State::INITED)
    vetoable_start();

  kernel::actor::ActorImpl* issuer = Actor::self()->get_impl();
  kernel::actor::simcall_blocking<void>([this, issuer, timeout] { this->get_impl()->wait_for(issuer, timeout); });
  state_ = State::FINISHED;
  on_completion(*Actor::self(), *this);
  this->release_dependencies();
  return this;
}

int Exec::wait_any_for(std::vector<ExecPtr>* execs, double timeout)
{
  std::unique_ptr<kernel::activity::ExecImpl* []> rexecs(new kernel::activity::ExecImpl*[execs->size()]);
  std::transform(begin(*execs), end(*execs), rexecs.get(),
                 [](const ExecPtr& exec) { return static_cast<kernel::activity::ExecImpl*>(exec->pimpl_.get()); });

  int changed_pos = simcall_execution_waitany_for(rexecs.get(), execs->size(), timeout);
  if (changed_pos != -1)
    execs->at(changed_pos)->release_dependencies();
  return changed_pos;
}

Exec* Exec::cancel()
{
  kernel::actor::simcall([this] { boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->cancel(); });
  state_ = State::CANCELED;
  return this;
}

/** @brief change the execution bound
 * This means changing the maximal amount of flops per second that it may consume, regardless of what the host may
 * deliver. Currently, this cannot be changed once the exec started.
 */
ExecPtr Exec::set_bound(double bound)
{
  xbt_assert(state_ == State::INITED, "Cannot change the bound of an exec after its start");
  bound_ = bound;
  return this;
}
ExecPtr Exec::set_timeout(double timeout) // XBT_ATTRIB_DEPRECATED_v329
{
  xbt_assert(state_ == State::INITED, "Cannot change the bound of an exec after its start");
  timeout_ = timeout;
  return this;
}

ExecPtr Exec::set_flops_amount(double flops_amount)
{
  xbt_assert(state_ == State::INITED, "Cannot change the flop_amount of an exec after its start");
  flops_amounts_.assign(1, flops_amount);
  Activity::set_remaining(flops_amounts_.front());
  return this;
}

ExecPtr Exec::set_flops_amounts(const std::vector<double>& flops_amounts)
{
  xbt_assert(state_ == State::INITED, "Cannot change the flops_amounts of an exec after its start");
  flops_amounts_ = flops_amounts;
  parallel_      = true;
  return this;
}

ExecPtr Exec::set_bytes_amounts(const std::vector<double>& bytes_amounts)
{
  xbt_assert(state_ == State::INITED, "Cannot change the bytes_amounts of an exec after its start");
  bytes_amounts_ = bytes_amounts;
  parallel_      = true;
  return this;
}

/** @brief Retrieve the host on which this activity takes place.
 *  If it runs on more than one host, only the first host is returned.
 */
Host* Exec::get_host() const
{
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_host();
}
unsigned int Exec::get_host_number() const
{
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_host_number();
}
double Exec::get_start_time() const
{
  return (pimpl_->surf_action_ == nullptr) ? -1 : pimpl_->surf_action_->get_start_time();
}
double Exec::get_finish_time() const
{
  return (pimpl_->surf_action_ == nullptr) ? -1 : pimpl_->surf_action_->get_finish_time();
}
double Exec::get_cost() const
{
  return (pimpl_->surf_action_ == nullptr) ? -1 : pimpl_->surf_action_->get_cost();
}

/** @brief  Change the execution priority, don't you think?
 *
 * An execution with twice the priority will get twice the amount of flops when the resource is shared.
 * The default priority is 1.
 *
 * Currently, this cannot be changed once the exec started. */
ExecPtr Exec::set_priority(double priority)
{
  xbt_assert(state_ == State::INITED, "Cannot change the priority of an exec after its start");
  priority_ = priority;
  return this;
}

/** @brief Change the host on which this activity takes place.
 *
 * The activity cannot be terminated already (but it may be started). */
ExecPtr Exec::set_host(Host* host)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED,
             "Cannot change the host of an exec once it's done (state: %d)", (int)state_);
  if (state_ == State::STARTED)
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->migrate(host);
  boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_host(host);
  return this;
}

ExecPtr Exec::set_hosts(const std::vector<Host*>& hosts)
{
  xbt_assert(state_ == State::INITED, "Cannot change the hosts of an exec once it's done (state: %d)", (int)state_);
  hosts_    = hosts;
  parallel_ = true;
  return this;
}

///////////// SEQUENTIAL EXECUTIONS ////////
Exec* Exec::start()
{
  if (is_parallel())
    kernel::actor::simcall([this] {
      (*boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_))
          .set_hosts(hosts_)
          .set_timeout(timeout_)
          .set_flops_amounts(flops_amounts_)
          .set_bytes_amounts(bytes_amounts_)
          .start();
    });
  else
    kernel::actor::simcall([this] {
      (*boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_))
          .set_name(get_name())
          .set_tracing_category(get_tracing_category())
          .set_sharing_penalty(1. / priority_)
          .set_bound(bound_)
          .set_flops_amount(flops_amounts_.front())
          .start();
    });
  state_ = State::STARTED;
  on_start(*Actor::self(), *this);
  return this;
}

double Exec::get_remaining() const
{
  if (is_parallel()) {
    XBT_WARN("Calling get_remaining() on a parallel execution is not allowed. Call get_remaining_ratio() instead.");
    return get_remaining_ratio();
  } else
    return kernel::actor::simcall(
        [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_remaining(); });
}

/** @brief Returns the ratio of elements that are still to do
 *
 * The returned value is between 0 (completely done) and 1 (nothing done yet).
 */
double Exec::get_remaining_ratio() const
{
  if (is_parallel())
    return kernel::actor::simcall(
        [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_par_remaining_ratio(); });
  else
    return kernel::actor::simcall(
        [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_seq_remaining_ratio(); });
}

} // namespace s4u
} // namespace simgrid
/* **************************** Public C interface *************************** */
void sg_exec_set_bound(sg_exec_t exec, double bound)
{
  exec->set_bound(bound);
}

double sg_exec_get_remaining(sg_exec_t exec)
{
  return exec->get_remaining();
}

void sg_exec_start(sg_exec_t exec)
{
  exec->start();
}

void sg_exec_wait(sg_exec_t exec)
{
  exec->wait_for(-1);
  exec->unref();
}

void sg_exec_wait_for(sg_exec_t exec, double timeout)
{
  exec->wait_for(timeout);
  exec->unref();
}
