/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/exec.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_exec, s4u_activity, "S4U asynchronous executions");

namespace simgrid {
namespace s4u {
xbt::signal<void(Exec const&)> Exec::on_start;
xbt::signal<void(Exec const&)> Exec::on_completion;

Exec::Exec(kernel::activity::ExecImplPtr pimpl)
{
  pimpl_ = pimpl;
}

void Exec::complete(Activity::State state)
{
  Activity::complete(state);
  on_completion(*this);
}

ExecPtr Exec::init()
{
  auto pimpl = kernel::activity::ExecImplPtr(new kernel::activity::ExecImpl());
  Host::on_state_change.connect([pimpl](s4u::Host const& h) {
    if (not h.is_on() && pimpl->state_ == kernel::activity::State::RUNNING &&
        std::find(pimpl->get_hosts().begin(), pimpl->get_hosts().end(), &h) != pimpl->get_hosts().end()) {
      pimpl->state_ = kernel::activity::State::FAILED;
      pimpl->post();
    }
  });
  return ExecPtr(pimpl->get_iface());
}

Exec* Exec::start()
{
  kernel::actor::simcall([this] {
    (*boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_))
        .set_name(get_name())
        .set_tracing_category(get_tracing_category())
        .start();
  });

  if (suspended_)
    pimpl_->suspend();

  state_      = State::STARTED;
  on_start(*this);
  return this;
}

ssize_t Exec::wait_any_for(const std::vector<ExecPtr>& execs, double timeout)
{
  std::vector<kernel::activity::ExecImpl*> rexecs(execs.size());
  std::transform(begin(execs), end(execs), begin(rexecs),
                 [](const ExecPtr& exec) { return static_cast<kernel::activity::ExecImpl*>(exec->pimpl_.get()); });

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ExecutionWaitanySimcall observer{issuer, rexecs, timeout};
  ssize_t changed_pos = kernel::actor::simcall_blocking(
      [&observer] {
        kernel::activity::ExecImpl::wait_any_for(observer.get_issuer(), observer.get_execs(), observer.get_timeout());
      },
      &observer);
  if (changed_pos != -1)
    execs.at(changed_pos)->complete(State::FINISHED);
  return changed_pos;
}

/** @brief change the execution bound
 * This means changing the maximal amount of flops per second that it may consume, regardless of what the host may
 * deliver. Currently, this cannot be changed once the exec started.
 */
ExecPtr Exec::set_bound(double bound)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the bound of an exec after its start");
  kernel::actor::simcall(
      [this, bound] { boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_bound(bound); });
  return this;
}

/** @brief  Change the execution priority, don't you think?
 *
 * An execution with twice the priority will get twice the amount of flops when the resource is shared.
 * The default priority is 1.
 *
 * Currently, this cannot be changed once the exec started. */
ExecPtr Exec::set_priority(double priority)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the priority of an exec after its start");
  kernel::actor::simcall([this, priority] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_sharing_penalty(1. / priority);
  });
  return this;
}

ExecPtr Exec::set_flops_amount(double flops_amount)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
      "Cannot change the flop_amount of an exec after its start");
  kernel::actor::simcall([this, flops_amount] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_flops_amount(flops_amount);
  });
  Activity::set_remaining(flops_amount);
  return this;
}

ExecPtr Exec::set_flops_amounts(const std::vector<double>& flops_amounts)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
      "Cannot change the flops_amounts of an exec after its start");
  kernel::actor::simcall([this, flops_amounts] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_flops_amounts(flops_amounts);
  });
  parallel_      = true;
  return this;
}

ExecPtr Exec::set_bytes_amounts(const std::vector<double>& bytes_amounts)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
      "Cannot change the bytes_amounts of an exec after its start");
  kernel::actor::simcall([this, bytes_amounts] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_bytes_amounts(bytes_amounts);
  });
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
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_start_time();
}

double Exec::get_finish_time() const
{
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_finish_time();
}

/** @brief Change the host on which this activity takes place.
 *
 * The activity cannot be terminated already (but it may be started). */
ExecPtr Exec::set_host(Host* host)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING || state_ == State::STARTED,
             "Cannot change the host of an exec once it's done (state: %s)", to_c_str(state_));

  if (state_ == State::STARTED)
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->migrate(host);

  kernel::actor::simcall(
      [this, host] { boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_host(host); });

  if (state_ == State::STARTING)
  // Setting the host may allow to start the activity, let's try
    vetoable_start();

  return this;
}

ExecPtr Exec::set_hosts(const std::vector<Host*>& hosts)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the hosts of an exec once it's done (state: %s)", to_c_str(state_));

  kernel::actor::simcall(
      [this, hosts] { boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_hosts(hosts); });
  parallel_ = true;

  // Setting the host may allow to start the activity, let's try
  if (state_ == State::STARTING)
     vetoable_start();

  return this;
}

double Exec::get_cost() const
{
  return (pimpl_->surf_action_ == nullptr) ? -1 : pimpl_->surf_action_->get_cost();
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

bool Exec::is_assigned() const
{
  return not boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_hosts().empty();
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
void sg_exec_set_bound(sg_exec_t exec, double bound)
{
  exec->set_bound(bound);
}

const char* sg_exec_get_name(const_sg_exec_t exec)
{
  return exec->get_cname();
}

void sg_exec_set_name(sg_exec_t exec, const char* name)
{
  exec->set_name(name);
}

void sg_exec_set_host(sg_exec_t exec, sg_host_t new_host)
{
  exec->set_host(new_host);
}

double sg_exec_get_remaining(const_sg_exec_t exec)
{
  return exec->get_remaining();
}

double sg_exec_get_remaining_ratio(const_sg_exec_t exec)
{
  return exec->get_remaining_ratio();
}

void sg_exec_start(sg_exec_t exec)
{
  exec->vetoable_start();
}

void sg_exec_cancel(sg_exec_t exec)
{
  exec->cancel();
  exec->unref();
}

int sg_exec_test(sg_exec_t exec)
{
  bool finished = exec->test();
  if (finished)
    exec->unref();
  return finished;
}

sg_error_t sg_exec_wait(sg_exec_t exec)
{
  return sg_exec_wait_for(exec, -1.0);
}

sg_error_t sg_exec_wait_for(sg_exec_t exec, double timeout)
{
  sg_error_t status = SG_OK;

  simgrid::s4u::ExecPtr s4u_exec(exec, false);
  try {
    s4u_exec->wait_for(timeout);
  } catch (const simgrid::TimeoutException&) {
    s4u_exec->add_ref(); // the wait_for timeouted, keep the exec alive
    status = SG_ERROR_TIMEOUT;
  } catch (const simgrid::CancelException&) {
    status = SG_ERROR_CANCELED;
  } catch (const simgrid::HostFailureException&) {
    status = SG_ERROR_HOST;
  }
  return status;
}

ssize_t sg_exec_wait_any(sg_exec_t* execs, size_t count)
{
  return sg_exec_wait_any_for(execs, count, -1.0);
}

ssize_t sg_exec_wait_any_for(sg_exec_t* execs, size_t count, double timeout)
{
  std::vector<simgrid::s4u::ExecPtr> s4u_execs;
  for (size_t i = 0; i < count; i++)
    s4u_execs.emplace_back(execs[i], false);

  ssize_t pos = simgrid::s4u::Exec::wait_any_for(s4u_execs, timeout);
  for (size_t i = 0; i < count; i++) {
    if (pos != -1 && static_cast<size_t>(pos) != i)
      s4u_execs[i]->add_ref();
  }
  return pos;
}
