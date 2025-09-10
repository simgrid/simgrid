/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/forward.h"
#include "src/kernel/activity/MemoryImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/internal_config.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/sthread/sthread.h"
#if HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif
#include "src/kernel/resource/HostImpl.hpp"

#include <boost/core/demangle.hpp>
#include <typeinfo>
#include <utility>

#include <mutex> // To terminate the actors in a thread-safe manner

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_actor, kernel, "Logging specific to Actor's kernel side");

namespace simgrid::kernel::actor {

/*------------------------- [ ActorIDTrait ] -------------------------*/
unsigned long ActorIDTrait::maxpid_ = 0;

ActorIDTrait::ActorIDTrait(const std::string& name, aid_t ppid) : name_(name), pid_(maxpid_++), ppid_(ppid) {}

ActorImpl* ActorImpl::self()
{
  const context::Context* self_context = context::Context::self();

  return (self_context != nullptr) ? self_context->get_actor() : nullptr;
}

ActorImpl::ActorImpl(const std::string& name, s4u::Host* host, aid_t ppid)
    : ActorIDTrait(name, ppid), host_(host), piface_(this)
{
  simcall_.issuer_ = this;
  stacksize_       = context::Context::stack_size;
  recorded_memory_accesses_ = activity::MemoryAccessImplPtr(new activity::MemoryAccessImpl(this));
}

ActorImpl::~ActorImpl()
{
  if (EngineImpl::has_instance() && not EngineImpl::get_instance()->is_maestro(this)) {
    s4u::Actor::on_destruction(*get_ciface());
    get_ciface()->on_this_destruction(*get_ciface());
  }
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

  auto* actor = new ActorImpl(name, host, /*ppid*/ -1);
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

  context->get_actor()->cleanup_from_self();
  context->attach_stop();
}

/** Whether this actor is actually maestro */
bool ActorImpl::is_maestro() const
{
  return context_->is_maestro();
}

void ActorImpl::cleanup_from_kernel()
{
  xbt_assert(s4u::Actor::is_maestro(), "Cleanup_from_kernel must be called in maestro context");

  auto* engine = EngineImpl::get_instance();
  if (engine->get_actor_by_pid(get_pid()) == nullptr)
    return; // Already cleaned

  engine->remove_actor(get_pid());
  if (host_ && host_actor_list_hook.is_linked())
    host_->get_impl()->remove_actor(this);
  if (not kernel_destroy_list_hook.is_linked())
    engine->add_actor_to_destroy_list(*this);

  undaemonize();
  s4u::Actor::on_termination(*get_ciface());
  get_ciface()->on_this_termination(*get_ciface());

  while (not mailboxes_.empty())
    mailboxes_.back()->set_receiver(nullptr);
}

/* Do all the cleanups from the actor context. Warning, the simcall mechanism was not reignited so doing simcalls in
 * this context is dangerous */
void ActorImpl::cleanup_from_self()
{
  xbt_assert(not ActorImpl::is_maestro(), "Cleanup_from_self called from maestro on '%s'", get_cname());
  set_to_be_freed();

  if (on_exit) {
    // Execute the termination callbacks
    bool failed = wannadie();
    for (auto exit_fun = on_exit->crbegin(); exit_fun != on_exit->crend(); ++exit_fun)
      (*exit_fun)(failed);
    on_exit.reset();
  }

  /* cancel non-blocking activities, in a thread-safe way. We cannot protect this state modification in a simcall
   * because the actor is dying, but we need to take care of sthread. */
  static std::mutex destruction_mutex;
  sthread_disable();
  destruction_mutex.lock();
  sthread_enable();

  while (not activities_.empty())
    activities_.begin()->get()->cancel(); // cancel() removes the activity from this collection

  sthread_disable();
  destruction_mutex.unlock();
  sthread_enable();

  XBT_DEBUG("%s@%s(%ld) should not run anymore", get_cname(), get_host()->get_cname(), get_pid());

  /* Unregister associated timers if any */
  if (kill_timer_ != nullptr) {
    kill_timer_->remove();
    kill_timer_ = nullptr;
  }
  if (simcall_.timeout_cb_) {
    simcall_.timeout_cb_->remove();
    simcall_.timeout_cb_ = nullptr;
  }

  /* maybe the actor was killed during a simcall, reset its observer */
  simcall_.observer_ = nullptr;

  set_wannadie();
}

void ActorImpl::exit()
{
  set_wannadie();
  suspended_ = false;
  exception_ = nullptr;

  while (not waiting_synchros_.empty()) {
    /* Take an extra reference on the activity object that may be unref by Comm::finish() or friends */
    activity::ActivityImplPtr activity = waiting_synchros_.back();
    waiting_synchros_.pop_back();
    
    activity->cancel();
    activity->set_state(activity::State::FAILED);
    activity->finish();

    activities_.erase(activity);
  }
  while (not activities_.empty())
    activities_.begin()->get()->cancel();

  // Forcefully kill the actor if its host is turned off. Not a HostFailureException because you should not survive that
  this->throw_exception(std::make_exception_ptr(ForcefulKillException(host_->is_on() ? "exited" : "host failed")));
}

void ActorImpl::kill(ActorImpl* actor) const
{
  xbt_assert(not actor->is_maestro(), "Killing maestro is a rather bad idea.");
  if (actor->wannadie()) {
    XBT_DEBUG("Ignoring request to kill actor %s@%s that is already dead", actor->get_cname(),
              actor->host_->get_cname());
    return;
  }

  XBT_DEBUG("Actor '%s'@%s is killing actor '%s'@%s", get_cname(), host_ ? host_->get_cname() : "", actor->get_cname(),
            actor->host_ ? actor->host_->get_cname() : "");

  actor->exit();

  if (actor == this)
    XBT_DEBUG("Damn, this is a suicide :(");
  else
    EngineImpl::get_instance()->add_actor_to_run_list(actor);
}

void ActorImpl::kill_all() const
{
  for (auto const& [_, actor] : EngineImpl::get_instance()->get_actor_list())
    if (actor != this)
      this->kill(actor);
}

void ActorImpl::set_kill_time(double kill_time)
{
  if (kill_time <= s4u::Engine::get_clock())
    return;
  XBT_DEBUG("Set kill time %f for actor %s@%s", kill_time, get_cname(), host_->get_cname());
  kill_timer_ = timer::Timer::set(kill_time, [this] {
    this->exit();
    kill_timer_ = nullptr;
    EngineImpl::get_instance()->add_actor_to_run_list(this);
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

  if (wannadie()) {
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
  if (not wannadie())
    smpi_switch_data_segment(get_iface());
#endif
  if (simgrid_mc_replay_show_backtraces)
    xbt_backtrace_display_current();
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
  xbt_assert(not this->is_maestro(), "Restarting maestro is not supported");

  XBT_DEBUG("Restarting actor %s on %s", get_cname(), host_->get_cname());

  // retrieve the arguments of the old actor
  ProcessArg args(host_, this);

  // kill the old actor
  context::Context::self()->get_actor()->kill(this);

  // start the new actor
  return create(&args)->get_ciface();
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

  if (wannadie()) {
    XBT_VERB("Ignoring request to resume an actor that is currently dying.");
    return;
  }

  if (not suspended_)
    return;
  suspended_ = false;

  /* resume the activities that were blocked when suspending the actor. */
  for (auto const& activity : activities_)
    activity->resume();
  if (waiting_synchros_.empty()) // Reschedule the actor if it was forcefully unscheduled in yield()
    EngineImpl::get_instance()->add_actor_to_run_list_no_check(this);

  XBT_OUT();
}

activity::ActivityImplPtr ActorImpl::join(const ActorImpl* actor, double timeout)
{
  activity::ActivityImplPtr sleep_activity = this->sleep(timeout);
  if (actor->wannadie() || actor->to_be_freed()) {
    if (sleep_activity->model_action_)
      sleep_activity->model_action_->finish(resource::Action::State::FINISHED);
  } else {
    actor->on_exit->emplace_back([sleep_activity](bool) {
      if (sleep_activity->model_action_)
        sleep_activity->model_action_->finish(resource::Action::State::FINISHED);
    });
  }
  return sleep_activity;
}

activity::ActivityImplPtr ActorImpl::sleep(double duration)
{
  if (not host_->is_on())
    throw_exception(std::make_exception_ptr(
        HostFailureException(XBT_THROW_POINT, "Host " + host_->get_name() + " failed, you cannot sleep there.")));

  auto* sleep_activity = new activity::SleepImpl();
  sleep_activity->set_name("sleep").set_host(host_).set_duration(duration).start();
  return activity::SleepImplPtr(sleep_activity);
}

void ActorImpl::throw_exception(std::exception_ptr e)
{
  exception_ = e;

  if (suspended_)
    resume();

  /* cancel the blocking synchros if any */
  while (not waiting_synchros_.empty()) {
    waiting_synchros_.back()->cancel();
    activities_.erase(waiting_synchros_.back());
    waiting_synchros_.pop_back();
  }
}

void ActorImpl::simcall_answer()
{
  auto* engine = EngineImpl::get_instance();
  if (not this->is_maestro()) {
    XBT_DEBUG("Answer simcall %s issued by %s (%p)", simcall_.get_cname(), get_cname(), this);
    xbt_assert(simcall_.call_ != Simcall::Type::NONE);
    simcall_.call_            = Simcall::Type::NONE;
    const auto& actors_to_run = engine->get_actors_to_run();
    xbt_assert(not XBT_LOG_ISENABLED(ker_actor, xbt_log_priority_debug) ||
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
  auto* actor = new ActorImpl(name, host, get_pid());

  intrusive_ptr_add_ref(actor);
  /* The on_creation() signal must be delayed until there, where the pid and everything is set */
  s4u::Actor::on_creation(*actor->get_ciface());

  return ActorImplPtr(actor);
}

ActorImpl* ActorImpl::start(const ActorCode& code)
{
  xbt_assert(code, "Cannot start an actor that does not have any code");
  xbt_assert(host_ != nullptr, "Cannot start an actor that is not located on an host");
  auto* engine = EngineImpl::get_instance();

  if (not host_->is_on()) {
    XBT_WARN("Cannot launch actor '%s' on failed host '%s'", get_cname(), host_->get_cname());
    intrusive_ptr_release(this);
    throw HostFailureException(XBT_THROW_POINT, "Cannot start actor on failed host.");
  }

  this->code_ = code;
  XBT_VERB("Create context %s", get_cname());
  context_.reset(engine->get_context_factory()->create_context(ActorCode(code), this));

  XBT_DEBUG("Start context '%s'", get_cname());

  /* Add the actor to its host's actor list */
  host_->get_impl()->add_actor(this);
  engine->add_actor(get_pid(), this);

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
    actor = parent_actor->init(name, host);
  else
    actor = self()->init(name, host);

  actor->piface_.set_data(data); /* actor data */

  actor->start(code);

  return actor;
}
ActorImplPtr ActorImpl::create(ProcessArg* args)
{
  ActorImplPtr actor    = ActorImpl::create(args->name, args->code, nullptr, args->host, nullptr);
  actor->restart_count_ = args->restart_count_;
  actor->set_properties(args->properties);
  if (args->on_exit)
    *actor->on_exit = *args->on_exit;
  if (args->kill_time >= 0)
    actor->set_kill_time(args->kill_time);
  if (args->auto_restart)
    actor->set_auto_restart(args->auto_restart);
  if (args->daemon_)
    actor->daemonize();
  return actor;
}
void ActorImpl::set_wannadie(bool value)
{
  XBT_DEBUG("Actor %s gonna die.", get_cname());
  iwannadie_ = value;
}

void create_maestro(const std::function<void()>& code)
{
  auto* engine = EngineImpl::get_instance();
  /* Create maestro actor and initialize it */
  auto* maestro = new ActorImpl(/*name*/ "", /*host*/ nullptr, /*ppid*/ -1);

  if (not code) {
    maestro->context_.reset(engine->get_context_factory()->create_context(ActorCode(), maestro));
  } else {
    maestro->context_.reset(engine->get_context_factory()->create_maestro(ActorCode(code), maestro));
  }

  maestro->simcall_.issuer_ = maestro;
  engine->set_maestro(maestro);
}

/** (in kernel mode) unpack the simcall and activate the handler */
void ActorImpl::simcall_handle(int times_considered)
{
  XBT_DEBUG("Handling simcall %p: %s(%ld) %s (times_considered:%d)", &simcall_, simcall_.issuer_->get_cname(),
            simcall_.issuer_->get_pid(),
            (simcall_.observer_ != nullptr ? simcall_.observer_->to_string().c_str() : simcall_.get_cname()),
            times_considered);
  if (simcall_.observer_ != nullptr)
    simcall_.observer_->prepare(times_considered);
  if (wannadie())
    return;

  xbt_assert(simcall_.call_ != Simcall::Type::NONE, "Asked to do the noop syscall on %s@%s", get_cname(),
             get_host()->get_cname());

  (*simcall_.code_)();
  if (simcall_.call_ == Simcall::Type::RUN_ANSWERED)
    simcall_answer();
}

} // namespace simgrid::kernel::actor
