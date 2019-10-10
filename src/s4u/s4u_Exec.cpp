/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

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

bool Exec::test()
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::FINISHED);

  if (state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED)
    this->start();

  if (simcall_execution_test(pimpl_)) {
    state_ = State::FINISHED;
    return true;
  }

  return false;
}

Exec* Exec::wait()
{
  if (state_ == State::INITED)
    start();
  simcall_execution_wait(pimpl_);
  state_ = State::FINISHED;
  on_completion(*Actor::self(), *this);
  return this;
}

Exec* Exec::wait_for(double)
{
  THROW_UNIMPLEMENTED;
}

int Exec::wait_any_for(std::vector<ExecPtr>* execs, double timeout)
{
  std::unique_ptr<kernel::activity::ExecImpl* []> rexecs(new kernel::activity::ExecImpl*[execs->size()]);
  std::transform(begin(*execs), end(*execs), rexecs.get(),
                 [](const ExecPtr& exec) { return static_cast<kernel::activity::ExecImpl*>(exec->pimpl_.get()); });
  return simcall_execution_waitany_for(rexecs.get(), execs->size(), timeout);
}

Exec* Exec::cancel()
{
  kernel::actor::simcall([this] { boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->cancel(); });
  state_ = State::CANCELED;
  return this;
}

void intrusive_ptr_release(simgrid::s4u::Exec* e)
{
  if (e->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
    std::atomic_thread_fence(std::memory_order_acquire);
    delete e;
  }
}

void intrusive_ptr_add_ref(simgrid::s4u::Exec* e)
{
  e->refcount_.fetch_add(1, std::memory_order_relaxed);
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
ExecPtr Exec::set_timeout(double timeout)
{
  xbt_assert(state_ == State::INITED, "Cannot change the bound of an exec after its start");
  timeout_ = timeout;
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

///////////// SEQUENTIAL EXECUTIONS ////////
ExecSeq::ExecSeq(sg_host_t host, double flops_amount) : Exec(), flops_amount_(flops_amount)
{
  Activity::set_remaining(flops_amount_);
  boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->set_host(host);
}

Exec* ExecSeq::start()
{
  kernel::actor::simcall([this] {
    (*boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_))
        .set_name(get_name())
        .set_tracing_category(get_tracing_category())
        .set_sharing_penalty(1. / priority_)
        .set_bound(bound_)
        .set_flops_amount(flops_amount_)
        .start();
  });
  state_ = State::STARTED;
  on_start(*Actor::self(), *this);
  return this;
}

/** @brief Returns whether the state of the exec is finished */
/** @brief Change the host on which this activity takes place.
 *
 * The activity cannot be terminated already (but it may be started). */
ExecPtr ExecSeq::set_host(Host* host)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED,
             "Cannot change the host of an exec once it's done (state: %d)", (int)state_);
  if (state_ == State::STARTED)
    boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->migrate(host);
  boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->set_host(host);
  return this;
}

/** @brief Returns the amount of flops that remain to be done */
double ExecSeq::get_remaining()
{
  return simgrid::kernel::actor::simcall(
      [this]() { return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->get_remaining(); });
}

/** @brief Returns the ratio of elements that are still to do
 *
 * The returned value is between 0 (completely done) and 1 (nothing done yet).
 */
double ExecSeq::get_remaining_ratio()
{
  return simgrid::kernel::actor::simcall([this]() {
    return boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_)->get_seq_remaining_ratio();
  });
}

///////////// PARALLEL EXECUTIONS ////////
ExecPar::ExecPar(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                 const std::vector<double>& bytes_amounts)
    : Exec(), hosts_(hosts), flops_amounts_(flops_amounts), bytes_amounts_(bytes_amounts)
{
}

Exec* ExecPar::start()
{
  kernel::actor::simcall([this] {
    (*boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_))
        .set_hosts(hosts_)
        .set_timeout(timeout_)
        .set_flops_amounts(flops_amounts_)
        .set_bytes_amounts(bytes_amounts_)
        .start();
  });
  state_ = State::STARTED;
  on_start(*Actor::self(), *this);
  return this;
}

double ExecPar::get_remaining_ratio()
{
  return kernel::actor::simcall(
      [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_par_remaining_ratio(); });
}

double ExecPar::get_remaining()
{
  XBT_WARN("Calling get_remaining() on a parallel execution is not allowed. Call get_remaining_ratio() instead.");
  return get_remaining_ratio();
}
} // namespace s4u
} // namespace simgrid
