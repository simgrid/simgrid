/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simcall.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/exec.h>
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_exec, s4u_activity, "S4U asynchronous executions");

namespace simgrid::s4u {
template <> xbt::signal<void(Exec&)> Activity_T<Exec>::on_veto             = xbt::signal<void(Exec&)>();
template <> xbt::signal<void(Exec const&)> Activity_T<Exec>::on_start      = xbt::signal<void(Exec const&)>();
template <> xbt::signal<void(Exec const&)> Activity_T<Exec>::on_completion = xbt::signal<void(Exec const&)>();
template <> xbt::signal<void(Exec const&)> Activity_T<Exec>::on_suspend    = xbt::signal<void(Exec const&)>();
template <> xbt::signal<void(Exec const&)> Activity_T<Exec>::on_resume     = xbt::signal<void(Exec const&)>();
template <> void Activity_T<Exec>::fire_on_start() const
{
  on_start(static_cast<const Exec&>(*this));
}
template <> void Activity_T<Exec>::fire_on_completion() const
{
  on_completion(static_cast<const Exec&>(*this));
}
template <> void Activity_T<Exec>::fire_on_suspend() const
{
  on_suspend(static_cast<const Exec&>(*this));
}
template <> void Activity_T<Exec>::fire_on_resume() const
{
  on_resume(static_cast<const Exec&>(*this));
}
template <> void Activity_T<Exec>::fire_on_veto()
{
  on_veto(static_cast<Exec&>(*this));
}
template <> void Activity_T<Exec>::on_start_cb(const std::function<void(Exec const&)>& cb)
{
  on_start.connect(cb);
}
template <> void Activity_T<Exec>::on_completion_cb(const std::function<void(Exec const&)>& cb)
{
  on_completion.connect(cb);
}
template <> void Activity_T<Exec>::on_suspend_cb(const std::function<void(Exec const&)>& cb)
{
  on_suspend.connect(cb);
}
template <> void Activity_T<Exec>::on_resume_cb(const std::function<void(Exec const&)>& cb)
{
  on_resume.connect(cb);
}
template <> void Activity_T<Exec>::on_veto_cb(const std::function<void(Exec&)>& cb)
{
  on_veto.connect(cb);
}

Exec::Exec(kernel::activity::ExecImplPtr pimpl)
{
  pimpl_ = pimpl;
}

void Exec::reset() const
{
  boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->reset();
}

ExecPtr Exec::init()
{
  auto pimpl = kernel::activity::ExecImplPtr(new kernel::activity::ExecImpl());
  return ExecPtr(static_cast<Exec*>(pimpl->get_iface()));
}

Exec* Exec::do_start()
{
  kernel::actor::simcall_answered([this] {
    auto pimpl = boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_);
    pimpl->set_name(get_name());
    pimpl->set_tracing_category(get_tracing_category());
    if (detached_)
      pimpl->detach();
    pimpl->start();
  });

  if (suspended_)
    pimpl_->suspend();

  state_      = State::STARTED;
  fire_on_start();
  fire_on_this_start();
  return this;
}

ExecPtr Exec::set_bound(double bound)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the bound of an exec after its start");
  kernel::actor::simcall_object_access(pimpl_.get(), [this, bound] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_bound(bound);
  });
  return this;
}

ExecPtr Exec::set_priority(double priority)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the priority of an exec after its start");
  kernel::actor::simcall_object_access(pimpl_.get(), [this, priority] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_sharing_penalty(1. / priority);
  });
  return this;
}

ExecPtr Exec::update_priority(double priority)
{
  kernel::actor::simcall_answered([this, priority] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->update_sharing_penalty(1. / priority);
  });
  return this;
}

ExecPtr Exec::set_flops_amount(double flops_amount)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
      "Cannot change the flop_amount of an exec after its start");
  kernel::actor::simcall_object_access(pimpl_.get(), [this, flops_amount] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_flops_amount(flops_amount);
  });
  set_remaining(flops_amount);
  return this;
}

ExecPtr Exec::set_flops_amounts(const std::vector<double>& flops_amounts)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
      "Cannot change the flops_amounts of an exec after its start");
  kernel::actor::simcall_object_access(pimpl_.get(), [this, flops_amounts] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_flops_amounts(flops_amounts);
  });
  parallel_      = true;
  return this;
}

ExecPtr Exec::set_bytes_amounts(const std::vector<double>& bytes_amounts)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
      "Cannot change the bytes_amounts of an exec after its start");
  kernel::actor::simcall_object_access(pimpl_.get(), [this, bytes_amounts] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_bytes_amounts(bytes_amounts);
  });
  parallel_      = true;
  return this;
}

ExecPtr Exec::set_thread_count(int thread_count)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the bytes_amounts of an exec after its start");
  xbt_assert(thread_count > 0, "The number of threads must be positive");

  kernel::actor::simcall_object_access(pimpl_.get(), [this, thread_count] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_thread_count(thread_count);
  });
  return this;
}

Host* Exec::get_host() const
{
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_host();
}
unsigned int Exec::get_host_number() const
{
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_host_number();
}

int Exec::get_thread_count() const
{
  return static_cast<kernel::activity::ExecImpl*>(pimpl_.get())->get_thread_count();
}

ExecPtr Exec::set_host(Host* host)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING || state_ == State::STARTED,
             "Cannot change the host of an exec once it's done (state: %s)", to_c_str(state_));

  if (state_ == State::STARTED)
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->migrate(host);

  kernel::actor::simcall_object_access(
      pimpl_.get(), [this, host] { boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_host(host); });

  if (state_ == State::STARTING)
    // Setting the host may allow to start the activity, let's try
    start();

  return this;
}

ExecPtr Exec::set_hosts(const std::vector<Host*>& hosts)
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTING,
             "Cannot change the hosts of an exec once it's done (state: %s)", to_c_str(state_));

  kernel::actor::simcall_object_access(pimpl_.get(), [this, hosts] {
    boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->set_hosts(hosts);
  });
  parallel_ = true;

  // Setting the host may allow to start the activity, let's try
  if (state_ == State::STARTING)
     start();

  return this;
}

ExecPtr Exec::unset_host()
{
  if (not is_assigned())
    throw std::invalid_argument(
        xbt::string_printf("Exec %s: the activity is not assigned to any host(s)", get_cname()));
  else {
    reset();

    if (state_ == State::STARTED)
      cancel();
    start();

    return this;
  }
}

double Exec::get_cost() const
{
  return (pimpl_->model_action_ == nullptr) ? -1 : pimpl_->model_action_->get_cost();
}

double Exec::get_remaining() const
{
  if (is_parallel()) {
    XBT_WARN("Calling get_remaining() on a parallel execution is not allowed. Call get_remaining_ratio() instead.");
    return get_remaining_ratio();
  } else
    return kernel::actor::simcall_answered(
        [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_remaining(); });
}

double Exec::get_remaining_ratio() const
{
  if (is_parallel())
    return kernel::actor::simcall_answered(
        [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_par_remaining_ratio(); });
  else
    return kernel::actor::simcall_answered(
        [this]() { return boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_seq_remaining_ratio(); });
}

bool Exec::is_assigned() const
{
  return not boost::static_pointer_cast<kernel::activity::ExecImpl>(pimpl_)->get_hosts().empty();
}
} // namespace simgrid::s4u

/* **************************** Public C interface *************************** */
int sg_exec_isinstance(sg_activity_t acti)
{
  return dynamic_cast<simgrid::s4u::Exec*>(acti) != nullptr;
}

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
  exec->start();
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
