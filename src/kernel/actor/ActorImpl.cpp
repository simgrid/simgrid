/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/remote/AppSide.hpp"
#if HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"

#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <utility>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix, "Logging specific to SIMIX (process)");

/**
 * @brief Returns the current agent.
 *
 * This functions returns the currently running SIMIX process.
 *
 * @return The SIMIX process
 */
smx_actor_t SIMIX_process_self()
{
  return simgrid::kernel::actor::ActorImpl::self();
}

namespace simgrid {
namespace kernel {
namespace actor {

static unsigned long maxpid = 0;
unsigned long get_maxpid()
{
  return maxpid;
}
unsigned long* get_maxpid_addr()
{
  return &maxpid;
}
ActorImpl* ActorImpl::by_pid(aid_t pid)
{
  return EngineImpl::get_instance()->get_actor_by_pid(pid);
}

ActorImpl* ActorImpl::self()
{
  const context::Context* self_context = context::Context::self();

  return (self_context != nullptr) ? self_context->get_actor() : nullptr;
}

ActorImpl::ActorImpl(xbt::string name, s4u::Host* host) : host_(host), name_(std::move(name)), piface_(this)
{
  pid_            = maxpid++;
  simcall_.issuer_ = this;
  stacksize_      = smx_context_stack_size;
}

ActorImpl::~ActorImpl()
{
  if (EngineImpl::has_instance() && not EngineImpl::get_instance()->is_maestro(this))
    s4u::Actor::on_destruction(*get_ciface());
}

/* Become an actor in the simulation
 *
 * Currently this can only be called by the main thread (once) and only work with some thread factories
 * (currently ThreadContextFactory).
 *
 * In the future, it might be extended in order to attach other threads created by a third party library.
 */

ActorImplPtr ActorImpl::attach(const std::string& name, void* data, s4u::Host* host)
{
  // This is mostly a copy/paste from create(), it'd be nice to share some code between those two functions.
  auto* engine = EngineImpl::get_instance();
  XBT_DEBUG("Attach actor %s on host '%s'", name.c_str(), host->get_cname());

  if (not host->is_on()) {
    XBT_WARN("Cannot attach actor '%s' on failed host '%s'", name.c_str(), host->get_cname());
    throw HostFailureException(XBT_THROW_POINT, "Cannot attach actor on failed host.");
  }

  auto* actor = new ActorImpl(xbt::string(name), host);
  /* Actor data */
  actor->piface_.set_data(data);
  actor->code_ = nullptr;

  XBT_VERB("Create context %s", actor->get_cname());
  actor->context_.reset(engine->get_context_factory()->attach(actor));

  /* Add the actor to it's host actor list */
  host->get_impl()->add_actor(actor);

  /* Now insert it in the global actor list and in the actors to run list */
  engine->add_actor(actor->get_pid(), actor);
  engine->add_actor_to_run_list_no_check(actor);
  intrusive_ptr_add_ref(actor);

  auto* context = dynamic_cast<context::AttachContext*>(actor->context_.get());
  xbt_assert(nullptr != context, "Not a suitable context");
  context->attach_start();

  /* The on_creation() signal must be delayed until there, where the pid and everything is set */
  s4u::Actor::on_creation(*actor->get_ciface());

  return ActorImplPtr(actor);
}
/** @brief Detach an actor attached with `attach()`
 *
 *  This is called when the current actor has finished its job.
 *  Used in the main thread, it waits for the simulation to finish before returning. When it returns, the other
 *  simulated actors and the maestro are destroyed.
 */
void ActorImpl::detach()
{
  auto* context = dynamic_cast<context::AttachContext*>(context::Context::self());
  xbt_assert(context != nullptr, "Not a suitable context");

  context->get_actor()->cleanup();
  context->attach_stop();
}

void ActorImpl::cleanup_from_simix()
{
  auto* engine = EngineImpl::get_instance();
  const std::lock_guard<std::mutex> lock(engine->get_mutex());
  engine->remove_actor(pid_);
  if (host_ && host_actor_list_hook.is_linked())
    host_->get_impl()->remove_actor(this);
  if (not kernel_destroy_list_hook.is_linked()) {
#if SIMGRID_HAVE_MC
    engine->add_dead_actor_to_dynar(this);
#endif
    engine->add_actor_to_destroy_list(*this);
  }
}

void ActorImpl::cleanup()
{
  finished_ = true;

  if (has_to_auto_restart() && not get_host()->is_on()) {
    XBT_DEBUG("Insert host %s to watched_hosts because it's off and %s needs to restart", get_host()->get_cname(),
              get_cname());
    watched_hosts().insert(get_host()->get_name());
  }

  if (on_exit) {
    // Execute the termination callbacks
    bool failed = context_->wannadie();
    for (auto exit_fun = on_exit->crbegin(); exit_fun != on_exit->crend(); ++exit_fun)
      (*exit_fun)(failed);
    on_exit.reset();
  }
  undaemonize();

  /* cancel non-blocking activities */
  for (auto activity : activities_)
    activity->cancel();
  activities_.clear();

  XBT_DEBUG("%s@%s(%ld) should not run anymore", get_cname(), get_host()->get_cname(), get_pid());

  if (EngineImpl::get_instance()->is_maestro(this)) /* Do not cleanup maestro */
    return;

  XBT_DEBUG("Cleanup actor %s (%p), waiting synchro %p", get_cname(), this, waiting_synchro_.get());

  /* Unregister associated timers if any */
  if (kill_timer_ != nullptr) {
    kill_timer_->remove();
    kill_timer_ = nullptr;
  }
  if (simcall_.timeout_cb_) {
    simcall_.timeout_cb_->remove();
    simcall_.timeout_cb_ = nullptr;
  }

  cleanup_from_simix();

  context_->set_wannadie(false); // don't let the simcall's yield() do a Context::stop(), to avoid infinite loops
  actor::simcall([this] { s4u::Actor::on_termination(*get_ciface()); });
  context_->set_wannadie();
}

void ActorImpl::exit()
{
  context_->set_wannadie();
  suspended_          = false;
  exception_          = nullptr;

  /* destroy the blocking synchro if any */
  if (waiting_synchro_ != nullptr) {
    waiting_synchro_->cancel();
    waiting_synchro_->state_ = activity::State::FAILED;

    activity::ExecImplPtr exec = boost::dynamic_pointer_cast<activity::ExecImpl>(waiting_synchro_);
    activity::CommImplPtr comm = boost::dynamic_pointer_cast<activity::CommImpl>(waiting_synchro_);

    if (exec != nullptr) {
      exec->clean_action();
    } else if (comm != nullptr) {
      comm->unregister_simcall(&simcall_);
    } else {
      activity::ActivityImplPtr(waiting_synchro_)->finish();
    }

    activities_.remove(waiting_synchro_);
    waiting_synchro_ = nullptr;
  }
  for (auto const& activity : activities_)
    activity->cancel();
  activities_.clear();

  // Forcefully kill the actor if its host is turned off. Not a HostFailureException because you should not survive that
  this->throw_exception(std::make_exception_ptr(ForcefulKillException(host_->is_on() ? "exited" : "host failed")));
}

void ActorImpl::kill(ActorImpl* actor) const
{
  xbt_assert(not EngineImpl::get_instance()->is_maestro(actor), "Killing maestro is a rather bad idea");
  if (actor->finished_) {
    XBT_DEBUG("Ignoring request to kill actor %s@%s that is already dead", actor->get_cname(),
              actor->host_->get_cname());
    return;
  }

  XBT_DEBUG("Actor '%s'@%s is killing actor '%s'@%s", get_cname(), host_ ? host_->get_cname() : "", actor->get_cname(),
            actor->host_ ? actor->host_->get_cname() : "");

  actor->exit();

  if (actor == this) {
    XBT_DEBUG("Go on, this is a suicide,");
  } else
    EngineImpl::get_instance()->add_actor_to_run_list(actor);
}

void ActorImpl::kill_all() const
{
  for (auto const& kv : EngineImpl::get_instance()->get_actor_list())
    if (kv.second != this)
      this->kill(kv.second);
}

void ActorImpl::set_kill_time(double kill_time)
{
  if (kill_time <= s4u::Engine::get_clock())
    return;
  XBT_DEBUG("Set kill time %f for actor %s@%s", kill_time, get_cname(), host_->get_cname());
  kill_timer_ = timer::Timer::set(kill_time, [this] {
    this->exit();
    kill_timer_ = nullptr;
  });
}

double ActorImpl::get_kill_time() const
{
  return kill_timer_ ? kill_timer_->get_date() : 0.0;
}

void ActorImpl::yield()
{
  XBT_DEBUG("Yield actor '%s'", get_cname());

  /* Go into sleep and return control to maestro */
  context_->suspend();

  /* Ok, maestro returned control to us */
  XBT_DEBUG("Control returned to me: '%s'", get_cname());

  if (context_->wannadie()) {
    XBT_DEBUG("Actor %s@%s is dead", get_cname(), host_->get_cname());
    context_->stop();
    THROW_IMPOSSIBLE;
  }

  if (suspended_) {
    XBT_DEBUG("Hey! I'm suspended.");
    xbt_assert(exception_ == nullptr, "Gasp! This exception may be lost by subsequent calls.");
    yield(); // Yield back to maestro without proceeding with my execution. I'll get rescheduled by resume()
  }

  if (exception_ != nullptr) {
    XBT_DEBUG("Wait, maestro left me an exception");
    std::exception_ptr exception = std::move(exception_);
    exception_                   = nullptr;
    try {
      std::rethrow_exception(std::move(exception));
    } catch (const simgrid::Exception& e) {
      e.rethrow_nested(XBT_THROW_POINT, boost::core::demangle(typeid(e).name()) + " raised in kernel mode.");
    }
  }

#if HAVE_SMPI
  if (not finished_)
    smpi_switch_data_segment(get_iface());
#endif
}

/** This actor will be terminated automatically when the last non-daemon actor finishes */
void ActorImpl::daemonize()
{
  if (not daemon_) {
    daemon_ = true;
    EngineImpl::get_instance()->add_daemon(this);
  }
}

void ActorImpl::undaemonize()
{
  if (daemon_) {
    daemon_ = false;
    EngineImpl::get_instance()->remove_daemon(this);
  }
}

s4u::Actor* ActorImpl::restart()
{
  xbt_assert(not EngineImpl::get_instance()->is_maestro(this), "Restarting maestro is not supported");

  XBT_DEBUG("Restarting actor %s on %s", get_cname(), host_->get_cname());

  // retrieve the arguments of the old actor
  ProcessArg arg(host_, this);

  // kill the old actor
  context::Context::self()->get_actor()->kill(this);

  // start the new actor
  ActorImplPtr actor = ActorImpl::create(arg.name, arg.code, arg.data, arg.host, nullptr);
  actor->set_properties(arg.properties);
  *actor->on_exit = std::move(*arg.on_exit);
  actor->set_kill_time(arg.kill_time);
  actor->set_auto_restart(arg.auto_restart);

  return actor->get_ciface();
}

void ActorImpl::suspend()
{
  if (suspended_) {
    XBT_DEBUG("Actor '%s' is already suspended", get_cname());
    return;
  }

  suspended_ = true;

  /* Suspend the activities associated with this actor. */
  for (auto const& activity : activities_)
    activity->suspend();
}

void ActorImpl::resume()
{
  XBT_IN("actor = %p", this);

  if (context_->wannadie()) {
    XBT_VERB("Ignoring request to suspend an actor that is currently dying.");
    return;
  }

  if (not suspended_)
    return;
  suspended_ = false;

  /* resume the activities that were blocked when suspending the actor. */
  for (auto const& activity : activities_)
    activity->resume();
  if (not waiting_synchro_) // Reschedule the actor if it was forcefully unscheduled in yield()
    EngineImpl::get_instance()->add_actor_to_run_list_no_check(this);

  XBT_OUT();
}

activity::ActivityImplPtr ActorImpl::join(const ActorImpl* actor, double timeout)
{
  activity::ActivityImplPtr sleep = this->sleep(timeout);
  actor->on_exit->emplace_back([sleep](bool) {
    if (sleep->surf_action_)
      sleep->surf_action_->finish(resource::Action::State::FINISHED);
  });
  return sleep;
}

activity::ActivityImplPtr ActorImpl::sleep(double duration)
{
  if (not host_->is_on())
    throw_exception(std::make_exception_ptr(HostFailureException(
        XBT_THROW_POINT, std::string("Host ") + host_->get_cname() + " failed, you cannot sleep there.")));

  auto sleep = new activity::SleepImpl();
  sleep->set_name("sleep").set_host(host_).set_duration(duration).start();
  return activity::SleepImplPtr(sleep);
}

void ActorImpl::throw_exception(std::exception_ptr e)
{
  exception_ = e;

  if (suspended_)
    resume();

  /* cancel the blocking synchro if any */
  if (waiting_synchro_) {
    waiting_synchro_->cancel();
    activities_.remove(waiting_synchro_);
    waiting_synchro_ = nullptr;
  }
}

void ActorImpl::simcall_answer()
{
  auto* engine = EngineImpl::get_instance();
  if (not engine->is_maestro(this)) {
    XBT_DEBUG("Answer simcall %s issued by %s (%p)", SIMIX_simcall_name(simcall_), get_cname(), this);
    xbt_assert(simcall_.call_ != simix::Simcall::NONE);
    simcall_.call_ = simix::Simcall::NONE;
    const auto& actors_to_run = engine->get_actors_to_run();
    xbt_assert(not XBT_LOG_ISENABLED(simix_process, xbt_log_priority_debug) ||
                   std::find(begin(actors_to_run), end(actors_to_run), this) == end(actors_to_run),
               "Actor %p should not exist in actors_to_run!", this);
    engine->add_actor_to_run_list_no_check(this);
  }
}

void ActorImpl::set_host(s4u::Host* dest)
{
  host_->get_impl()->remove_actor(this);
  host_ = dest;
  dest->get_impl()->add_actor(this);
}

ActorImplPtr ActorImpl::init(const std::string& name, s4u::Host* host) const
{
  auto* actor = new ActorImpl(xbt::string(name), host);
  actor->set_ppid(this->pid_);

  intrusive_ptr_add_ref(actor);
  /* The on_creation() signal must be delayed until there, where the pid and everything is set */
  s4u::Actor::on_creation(*actor->get_ciface());

  return ActorImplPtr(actor);
}

ActorImpl* ActorImpl::start(const ActorCode& code)
{
  xbt_assert(code && host_ != nullptr, "Invalid parameters");
  auto* engine = EngineImpl::get_instance();

  if (not host_->is_on()) {
    XBT_WARN("Cannot launch actor '%s' on failed host '%s'", name_.c_str(), host_->get_cname());
    intrusive_ptr_release(this);
    throw HostFailureException(XBT_THROW_POINT, "Cannot start actor on failed host.");
  }

  this->code_ = code;
  XBT_VERB("Create context %s", get_cname());
  context_.reset(engine->get_context_factory()->create_context(ActorCode(code), this));

  XBT_DEBUG("Start context '%s'", get_cname());

  /* Add the actor to its host's actor list */
  host_->get_impl()->add_actor(this);
  engine->add_actor(pid_, this);

  /* Now insert it in the global actor list and in the actor to run list */
  engine->add_actor_to_run_list_no_check(this);

  return this;
}

ActorImplPtr ActorImpl::create(const std::string& name, const ActorCode& code, void* data, s4u::Host* host,
                               const ActorImpl* parent_actor)
{
  XBT_DEBUG("Start actor %s@'%s'", name.c_str(), host->get_cname());

  ActorImplPtr actor;
  if (parent_actor != nullptr)
    actor = parent_actor->init(xbt::string(name), host);
  else
    actor = self()->init(xbt::string(name), host);

  /* actor data */
  actor->piface_.set_data(data);

  actor->start(code);

  return actor;
}

void create_maestro(const std::function<void()>& code)
{
  auto* engine = EngineImpl::get_instance();
  /* Create maestro actor and initialize it */
  auto* maestro = new ActorImpl(xbt::string(""), /*host*/ nullptr);

  if (not code) {
    maestro->context_.reset(engine->get_context_factory()->create_context(ActorCode(), maestro));
  } else {
    maestro->context_.reset(engine->get_context_factory()->create_maestro(ActorCode(code), maestro));
  }

  maestro->simcall_.issuer_ = maestro;
  EngineImpl::get_instance()->set_maestro(maestro);
}

} // namespace actor
} // namespace kernel
} // namespace simgrid

/* needs to be public and without simcall because it is called
   by exceptions and logging events */
const char* SIMIX_process_self_get_name()
{
  return SIMIX_is_maestro() ? "maestro" : simgrid::kernel::actor::ActorImpl::self()->get_cname();
}

/** @brief Returns the process from PID. */
smx_actor_t SIMIX_process_from_PID(aid_t pid) // XBT_ATTRIB_DEPRECATD_v331
{
  return simgrid::kernel::actor::ActorImpl::by_pid(pid);
}
