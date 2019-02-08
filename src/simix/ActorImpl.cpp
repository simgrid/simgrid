/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "smx_private.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/remote/Client.hpp"
#include "src/simix/smx_host_private.hpp"
#include "src/simix/smx_io_private.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"

#include <boost/range/algorithm.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix, "Logging specific to SIMIX (process)");

static unsigned long simix_process_maxpid = 0;

/**
 * @brief Returns the current agent.
 *
 * This functions returns the currently running SIMIX process.
 *
 * @return The SIMIX process
 */
smx_actor_t SIMIX_process_self()
{
  smx_context_t self_context = simgrid::kernel::context::Context::self();

  return (self_context != nullptr) ? self_context->get_actor() : nullptr;
}

/**
 * @brief Returns whether a process has pending asynchronous communications.
 * @return true if there are asynchronous communications in this process
 */
int SIMIX_process_has_pending_comms(smx_actor_t process) {

  return process->comms.size() > 0;
}

/**
 * @brief Moves a process to the list of processes to destroy.
 */
void SIMIX_process_cleanup(smx_actor_t process)
{
  XBT_DEBUG("Cleanup process %s (%p), waiting synchro %p", process->get_cname(), process,
            process->waiting_synchro.get());

  simix_global->mutex.lock();

  simix_global->process_list.erase(process->pid_);
  if (process->host_ && process->host_process_list_hook.is_linked())
    simgrid::xbt::intrusive_erase(process->host_->pimpl_->process_list_, *process);
  if (not process->smx_destroy_list_hook.is_linked()) {
#if SIMGRID_HAVE_MC
    xbt_dynar_push_as(simix_global->dead_actors_vector, smx_actor_t, process);
#endif
    simix_global->process_to_destroy.push_back(*process);
  }
  process->context_->iwannadie = false;

  simix_global->mutex.unlock();
}

/**
 * Garbage collection
 *
 * Should be called some time to time to free the memory allocated for processes that have finished (or killed).
 */
void SIMIX_process_empty_trash()
{
  while (not simix_global->process_to_destroy.empty()) {
    smx_actor_t process = &simix_global->process_to_destroy.front();
    simix_global->process_to_destroy.pop_front();
    XBT_DEBUG("Getting rid of %p",process);
    intrusive_ptr_release(process);
  }
#if SIMGRID_HAVE_MC
  xbt_dynar_reset(simix_global->dead_actors_vector);
#endif
}

namespace simgrid {
namespace kernel {
namespace actor {

ActorImpl::ActorImpl(simgrid::xbt::string name, simgrid::s4u::Host* host) : name_(name), host_(host), piface_(this)
{
  pid_ = simix_process_maxpid++;
  simcall.issuer = this;
}

ActorImpl::~ActorImpl()
{
  delete this->context_;
}

void ActorImpl::set_kill_time(double kill_time)
{
  if (kill_time <= SIMIX_get_clock())
    return;
  XBT_DEBUG("Set kill time %f for process %s@%s", kill_time, get_cname(), host_->get_cname());
  kill_timer = SIMIX_timer_set(kill_time, [this] {
    SIMIX_process_kill(this, nullptr);
    kill_timer = nullptr;
  });
}

static void dying_daemon(int /*exit_status*/, void* data)
{
  std::vector<ActorImpl*>* vect = &simix_global->daemons;

  auto it = std::find(vect->begin(), vect->end(), static_cast<ActorImpl*>(data));
  xbt_assert(it != vect->end(), "The dying daemon is not a daemon after all. Please report that bug.");

  /* Don't move the whole content since we don't really care about the order */
  std::swap(*it, vect->back());
  vect->pop_back();
}

/** This process will be terminated automatically when the last non-daemon process finishes */
void ActorImpl::daemonize()
{
  if (not daemon_) {
    daemon_ = true;
    simix_global->daemons.push_back(this);
    SIMIX_process_on_exit(this, dying_daemon, this);
  }
}

simgrid::s4u::Actor* ActorImpl::restart()
{
  XBT_DEBUG("Restarting process %s on %s", get_cname(), host_->get_cname());

  // retrieve the arguments of the old process
  simgrid::kernel::actor::ProcessArg arg = ProcessArg(host_, this);

  // kill the old process
  SIMIX_process_kill(this, (this == simix_global->maestro_process) ? this : SIMIX_process_self());

  // start the new process
  ActorImplPtr actor =
      ActorImpl::create(arg.name, std::move(arg.code), arg.data, arg.host, arg.properties.get(), nullptr);
  actor->set_kill_time(arg.kill_time);
  actor->set_auto_restart(arg.auto_restart);

  return actor->ciface();
}

smx_activity_t ActorImpl::suspend(ActorImpl* issuer)
{
  if (suspended_) {
    XBT_DEBUG("Actor '%s' is already suspended", get_cname());
    return nullptr;
  }

  suspended_ = true;

  /* If we are suspending another actor that is waiting on a sync, suspend its synchronization. */
  if (this != issuer) {
    if (waiting_synchro)
      waiting_synchro->suspend();
    /* If the other actor is not waiting, its suspension is delayed to when the actor is rescheduled. */

    return nullptr;
  } else {
    return activity::ExecImplPtr(new activity::ExecImpl("suspend", "", nullptr, this->host_))->start(0.0, 1.0, 0.0);
  }
}

void ActorImpl::resume()
{
  XBT_IN("process = %p", this);

  if (context_->iwannadie) {
    XBT_VERB("Ignoring request to suspend an actor that is currently dying.");
    return;
  }

  if (not suspended_)
    return;
  suspended_ = false;

  /* resume the synchronization that was blocking the resumed actor. */
  if (waiting_synchro)
    waiting_synchro->resume();

  XBT_OUT();
}

smx_activity_t ActorImpl::sleep(double duration)
{
  if (host_->is_off())
    throw_exception(std::make_exception_ptr(simgrid::HostFailureException(
        XBT_THROW_POINT, std::string("Host ") + std::string(host_->get_cname()) + " failed, you cannot sleep there.")));

  return simgrid::kernel::activity::SleepImplPtr(new simgrid::kernel::activity::SleepImpl("sleep", host_))
      ->start(duration);
}

void ActorImpl::throw_exception(std::exception_ptr e)
{
  exception = e;

  if (suspended_)
    resume();

  /* cancel the blocking synchro if any */
  if (waiting_synchro) {

    simgrid::kernel::activity::ExecImplPtr exec =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(waiting_synchro);
    if (exec != nullptr && exec->surf_action_)
      exec->surf_action_->cancel();

    simgrid::kernel::activity::CommImplPtr comm =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(waiting_synchro);
    if (comm != nullptr) {
      comms.remove(comm);
      comm->cancel();
    }

    simgrid::kernel::activity::SleepImplPtr sleep =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(waiting_synchro);
    if (sleep != nullptr) {
      SIMIX_process_sleep_destroy(waiting_synchro);
      if (std::find(begin(simix_global->process_to_run), end(simix_global->process_to_run), this) ==
              end(simix_global->process_to_run) &&
          this != SIMIX_process_self()) {
        XBT_DEBUG("Inserting %s in the to_run list", get_cname());
        simix_global->process_to_run.push_back(this);
      }
    }

    simgrid::kernel::activity::RawImplPtr raw =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::RawImpl>(waiting_synchro);
    if (raw != nullptr) {
      SIMIX_synchro_stop_waiting(this, &simcall);
    }

    simgrid::kernel::activity::IoImplPtr io =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::IoImpl>(waiting_synchro);
    if (io != nullptr) {
      delete io.get();
    }
  }
  waiting_synchro = nullptr;
}

void ActorImpl::set_host(sg_host_t dest)
{
  simgrid::xbt::intrusive_erase(host_->pimpl_->process_list_, *this);
  host_ = dest;
  dest->pimpl_->process_list_.push_back(*this);
}

ActorImplPtr ActorImpl::create(std::string name, simgrid::simix::ActorCode code, void* data, simgrid::s4u::Host* host,
                               std::unordered_map<std::string, std::string>* properties, smx_actor_t parent_actor)
{

  XBT_DEBUG("Start actor %s@'%s'", name.c_str(), host->get_cname());

  if (host->is_off()) {
    XBT_WARN("Cannot launch actor '%s' on failed host '%s'", name.c_str(), host->get_cname());
    return nullptr;
  }

  ActorImpl* actor = new simgrid::kernel::actor::ActorImpl(simgrid::xbt::string(name), host);

  xbt_assert(code && host != nullptr, "Invalid parameters");
  /* actor data */
  actor->set_user_data(data);
  actor->code = code;

  if (parent_actor != nullptr)
    actor->ppid_ = parent_actor->pid_;

  XBT_VERB("Create context %s", actor->get_cname());
  actor->context_ = SIMIX_context_new(std::move(code), &SIMIX_process_cleanup, actor);

  /* Add properties */
  if (properties != nullptr)
    for (auto const& kv : *properties)
      actor->set_property(kv.first, kv.second);

  /* Add the process to its host's process list */
  host->pimpl_->process_list_.push_back(*actor);

  XBT_DEBUG("Start context '%s'", actor->get_cname());

  /* Now insert it in the global process list and in the process to run list */
  simix_global->process_list[actor->pid_] = actor;
  XBT_DEBUG("Inserting %s(%s) in the to_run list", actor->get_cname(), host->get_cname());
  simix_global->process_to_run.push_back(actor);
  intrusive_ptr_add_ref(actor);

  /* The on_creation() signal must be delayed until there, where the pid and everything is set */
  simgrid::s4u::Actor::on_creation(actor->iface());

  return ActorImplPtr(actor);
}

void create_maestro(simgrid::simix::ActorCode code)
{
  /* Create maestro process and initialize it */
  smx_actor_t maestro = new simgrid::kernel::actor::ActorImpl(simgrid::xbt::string(""), /*host*/ nullptr);

  if (not code) {
    maestro->context_ = SIMIX_context_new(simgrid::simix::ActorCode(), nullptr, maestro);
  } else {
    maestro->context_ = simix_global->context_factory->create_maestro(code, maestro);
  }

  maestro->simcall.issuer       = maestro;
  simix_global->maestro_process = maestro;
}

} // namespace actor
} // namespace kernel
}

smx_actor_t SIMIX_process_attach(const char* name, void* data, const char* hostname,
                                 std::unordered_map<std::string, std::string>* properties, smx_actor_t parent_process)
{
  // This is mostly a copy/paste from SIMIX_process_new(),
  // it'd be nice to share some code between those two functions.

  sg_host_t host = sg_host_by_name(hostname);
  XBT_DEBUG("Attach process %s on host '%s'", name, hostname);

  if (host->is_off()) {
    XBT_WARN("Cannot launch process '%s' on failed host '%s'", name, hostname);
    return nullptr;
  }

  smx_actor_t actor = new simgrid::kernel::actor::ActorImpl(simgrid::xbt::string(name), host);
  /* Actor data */
  actor->set_user_data(data);
  actor->code = nullptr;

  if (parent_process != nullptr)
    actor->ppid_ = parent_process->pid_;

  XBT_VERB("Create context %s", actor->get_cname());
  xbt_assert(simix_global != nullptr, "simix is not initialized, please call MSG_init first");
  actor->context_ = simix_global->context_factory->attach(&SIMIX_process_cleanup, actor);

  /* Add properties */
  if (properties != nullptr)
    for (auto const& kv : *properties)
      actor->set_property(kv.first, kv.second);

  /* Add the process to it's host process list */
  host->pimpl_->process_list_.push_back(*actor);

  /* Now insert it in the global process list and in the process to run list */
  simix_global->process_list[actor->pid_] = actor;
  XBT_DEBUG("Inserting %s(%s) in the to_run list", actor->get_cname(), host->get_cname());
  simix_global->process_to_run.push_back(actor);
  intrusive_ptr_add_ref(actor);

  auto* context = dynamic_cast<simgrid::kernel::context::AttachContext*>(actor->context_);
  xbt_assert(nullptr != context, "Not a suitable context");
  context->attach_start();

  /* The on_creation() signal must be delayed until there, where the pid and everything is set */
  simgrid::s4u::ActorPtr tmp = actor->iface(); // Passing this directly to on_creation will lead to crashes
  simgrid::s4u::Actor::on_creation(tmp);

  return actor;
}

void SIMIX_process_detach()
{
  auto* context = dynamic_cast<simgrid::kernel::context::AttachContext*>(simgrid::kernel::context::Context::self());
  if (context == nullptr)
    xbt_die("Not a suitable context");

  SIMIX_process_cleanup(context->get_actor());
  context->attach_stop();
}

/**
 * @brief Executes the processes from simix_global->process_to_run.
 *
 * The processes of simix_global->process_to_run are run (in parallel if
 * possible).  On exit, simix_global->process_to_run is empty, and
 * simix_global->process_that_ran contains the list of processes that just ran.
 * The two lists are swapped so, be careful when using them before and after a
 * call to this function.
 */
void SIMIX_process_runall()
{
  SIMIX_context_runall();

  simix_global->process_to_run.swap(simix_global->process_that_ran);
  simix_global->process_to_run.clear();
}

/**
 * @brief Internal function to kill a SIMIX process.
 *
 * This function may be called when a SIMCALL_PROCESS_KILL simcall occurs,
 * or directly for SIMIX internal purposes.
 *
 * @param actor poor victim
 * @param issuer the actor which has sent the PROCESS_KILL. Important to not schedule twice the same actor.
 */
void SIMIX_process_kill(smx_actor_t actor, smx_actor_t issuer)
{

  if (actor->finished_) {
    XBT_DEBUG("Ignoring request to kill process %s@%s that is already dead", actor->get_cname(),
              actor->host_->get_cname());
    return;
  }

  XBT_DEBUG("Actor '%s'@%s is killing actor '%s'@%s", issuer == nullptr ? "(null)" : issuer->get_cname(),
            (issuer == nullptr || issuer->host_ == nullptr ? "(null)" : issuer->host_->get_cname()), actor->get_cname(),
            actor->host_->get_cname());

  actor->context_->iwannadie = true;
  actor->blocked_            = false;
  actor->suspended_          = false;
  actor->exception           = nullptr;

  // Forcefully kill the actor if its host is turned off. Not an HostFailureException because you should not survive that
  if (actor->host_->is_off())
    actor->throw_exception(std::make_exception_ptr(simgrid::kernel::context::StopRequest("host failed")));

  /* destroy the blocking synchro if any */
  if (actor->waiting_synchro != nullptr) {

    simgrid::kernel::activity::ExecImplPtr exec =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(actor->waiting_synchro);
    simgrid::kernel::activity::CommImplPtr comm =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(actor->waiting_synchro);
    simgrid::kernel::activity::SleepImplPtr sleep =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(actor->waiting_synchro);
    simgrid::kernel::activity::RawImplPtr raw =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::RawImpl>(actor->waiting_synchro);
    simgrid::kernel::activity::IoImplPtr io =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::IoImpl>(actor->waiting_synchro);

    if (exec != nullptr) {
      if (exec->surf_action_) {
        exec->surf_action_->cancel();
        exec->surf_action_->unref();
        exec->surf_action_ = nullptr;
      }
    } else if (comm != nullptr) {
      actor->comms.remove(actor->waiting_synchro);
      comm->cancel();
      // Remove first occurrence of &process->simcall:
      auto i = boost::range::find(actor->waiting_synchro->simcalls_, &actor->simcall);
      if (i != actor->waiting_synchro->simcalls_.end())
        actor->waiting_synchro->simcalls_.remove(&actor->simcall);
    } else if (sleep != nullptr) {
      if (sleep->surf_action_)
        sleep->surf_action_->cancel();
      sleep->post();
    } else if (raw != nullptr) {
      SIMIX_synchro_stop_waiting(actor, &actor->simcall);

    } else if (io != nullptr) {
      delete io.get();
    } else {
      simgrid::kernel::activity::ActivityImplPtr activity = actor->waiting_synchro;
      xbt_die("Activity %s is of unknown type %s", activity->name_.c_str(),
              simgrid::xbt::demangle(typeid(activity).name()).get());
    }

    actor->waiting_synchro = nullptr;
  }
  if (std::find(begin(simix_global->process_to_run), end(simix_global->process_to_run), actor) ==
          end(simix_global->process_to_run) &&
      actor != issuer) {
    XBT_DEBUG("Inserting %s in the to_run list", actor->get_cname());
    simix_global->process_to_run.push_back(actor);
  }
}

/** @deprecated When this function gets removed, also remove the xbt_ex class, that is only there to help users to
 * transition */
void SIMIX_process_throw(smx_actor_t actor, xbt_errcat_t cat, int value, const char* msg)
{
  SMX_EXCEPTION(actor, cat, value, msg);

  if (actor->suspended_)
    actor->resume();

  /* cancel the blocking synchro if any */
  if (actor->waiting_synchro) {

    simgrid::kernel::activity::ExecImplPtr exec =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(actor->waiting_synchro);
    if (exec != nullptr && exec->surf_action_)
      exec->surf_action_->cancel();

    simgrid::kernel::activity::CommImplPtr comm =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(actor->waiting_synchro);
    if (comm != nullptr) {
      actor->comms.remove(comm);
      comm->cancel();
    }

    simgrid::kernel::activity::SleepImplPtr sleep =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(actor->waiting_synchro);
    if (sleep != nullptr) {
      SIMIX_process_sleep_destroy(actor->waiting_synchro);
      if (std::find(begin(simix_global->process_to_run), end(simix_global->process_to_run), actor) ==
              end(simix_global->process_to_run) &&
          actor != SIMIX_process_self()) {
        XBT_DEBUG("Inserting %s in the to_run list", actor->get_cname());
        simix_global->process_to_run.push_back(actor);
      }
    }

    simgrid::kernel::activity::RawImplPtr raw =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::RawImpl>(actor->waiting_synchro);
    if (raw != nullptr) {
      SIMIX_synchro_stop_waiting(actor, &actor->simcall);
    }

    simgrid::kernel::activity::IoImplPtr io =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::IoImpl>(actor->waiting_synchro);
    if (io != nullptr) {
      delete io.get();
    }
  }
  actor->waiting_synchro = nullptr;
}

/**
 * @brief Kills all running processes.
 * @param issuer this one will not be killed
 */
void SIMIX_process_killall(smx_actor_t issuer)
{
  for (auto const& kv : simix_global->process_list)
    if (kv.second != issuer)
      SIMIX_process_kill(kv.second, issuer);
}


void simcall_HANDLER_process_suspend(smx_simcall_t simcall, smx_actor_t actor)
{
  smx_activity_t sync_suspend = actor->suspend(simcall->issuer);

  if (actor != simcall->issuer) {
    SIMIX_simcall_answer(simcall);
  } else {
    sync_suspend->simcalls_.push_back(simcall);
    actor->waiting_synchro = sync_suspend;
    actor->waiting_synchro->suspend();
  }
  /* If we are suspending ourselves, then just do not finish the simcall now */
}

int SIMIX_process_get_maxpid() {
  return simix_process_maxpid;
}

int SIMIX_process_count()
{
  return simix_global->process_list.size();
}

void* SIMIX_process_self_get_data()
{
  smx_actor_t self = SIMIX_process_self();

  if (self == nullptr) {
    return nullptr;
  }
  return self->get_user_data();
}

void SIMIX_process_self_set_data(void *data)
{
  SIMIX_process_self()->set_user_data(data);
}


/* needs to be public and without simcall because it is called
   by exceptions and logging events */
const char* SIMIX_process_self_get_name() {

  smx_actor_t process = SIMIX_process_self();
  if (process == nullptr || process == simix_global->maestro_process)
    return "maestro";

  return process->get_cname();
}

void simcall_HANDLER_process_join(smx_simcall_t simcall, smx_actor_t process, double timeout)
{
  if (process->finished_) {
    // The joined process is already finished, just wake up the issuer process right away
    simcall_process_sleep__set__result(simcall, SIMIX_DONE);
    SIMIX_simcall_answer(simcall);
    return;
  }
  smx_activity_t sync = SIMIX_process_join(simcall->issuer, process, timeout);
  sync->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = sync;
}

smx_activity_t SIMIX_process_join(smx_actor_t issuer, smx_actor_t process, double timeout)
{
  smx_activity_t res = issuer->sleep(timeout);
  intrusive_ptr_add_ref(res.get());
  SIMIX_process_on_exit(process,
                        [](int, void* arg) {
                          auto sleep = static_cast<simgrid::kernel::activity::SleepImpl*>(arg);
                          if (sleep->surf_action_)
                            sleep->surf_action_->finish(simgrid::kernel::resource::Action::State::FINISHED);
                          intrusive_ptr_release(sleep);
                        },
                        res.get());
  return res;
}

void simcall_HANDLER_process_sleep(smx_simcall_t simcall, double duration)
{
  if (MC_is_active() || MC_record_replay_is_active()) {
    MC_process_clock_add(simcall->issuer, duration);
    simcall_process_sleep__set__result(simcall, SIMIX_DONE);
    SIMIX_simcall_answer(simcall);
    return;
  }
  smx_activity_t sync = simcall->issuer->sleep(duration);
  sync->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = sync;
}

void SIMIX_process_sleep_destroy(smx_activity_t synchro)
{
  XBT_DEBUG("Destroy sleep synchro %p", synchro.get());
  simgrid::kernel::activity::SleepImplPtr sleep =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(synchro);

  if (sleep->surf_action_) {
    sleep->surf_action_->unref();
    sleep->surf_action_ = nullptr;
  }
}

/**
 * @brief Calling this function makes the process to yield.
 *
 * Only the current process can call this function, giving back the control to maestro.
 *
 * @param self the current process
 */
void SIMIX_process_yield(smx_actor_t self)
{
  XBT_DEBUG("Yield actor '%s'", self->get_cname());

  /* Go into sleep and return control to maestro */
  self->context_->suspend();

  /* Ok, maestro returned control to us */
  XBT_DEBUG("Control returned to me: '%s'", self->get_cname());

  if (self->context_->iwannadie) {

    XBT_DEBUG("Process %s@%s is dead", self->get_cname(), self->host_->get_cname());
    // throw simgrid::kernel::context::StopRequest(); Does not seem to properly kill the actor
    self->context_->stop();
    THROW_IMPOSSIBLE;
  }

  if (self->suspended_) {
    XBT_DEBUG("Hey! I'm suspended.");
    xbt_assert(self->exception != nullptr, "Gasp! This exception may be lost by subsequent calls.");
    self->suspended_ = false;
    self->suspend(self);
  }

  if (self->exception != nullptr) {
    XBT_DEBUG("Wait, maestro left me an exception");
    std::exception_ptr exception = std::move(self->exception);
    self->exception = nullptr;
    std::rethrow_exception(std::move(exception));
  }

  if (SMPI_switch_data_segment && not self->finished_) {
    SMPI_switch_data_segment(self->iface());
  }
}

/** @brief Returns the list of processes to run. */
const std::vector<smx_actor_t>& simgrid::simix::process_get_runnable()
{
  return simix_global->process_to_run;
}

/** @brief Returns the process from PID. */
smx_actor_t SIMIX_process_from_PID(aid_t PID)
{
  auto actor = simix_global->process_list.find(PID);
  return actor == simix_global->process_list.end() ? nullptr : actor->second;
}

void SIMIX_process_on_exit(smx_actor_t actor, int_f_pvoid_pvoid_t fun, void* data)
{
  SIMIX_process_on_exit(actor, [fun](int a, void* b) { fun((void*)(intptr_t)a, b); }, data);
}

void SIMIX_process_on_exit(smx_actor_t actor, std::function<void(int, void*)> fun, void* data)
{
  xbt_assert(actor, "current process not found: are you in maestro context ?");

  actor->on_exit.emplace_back(s_smx_process_exit_fun_t{fun, data});
}

/** @brief Restart a process, starting it again from the beginning. */
/**
 * @ingroup simix_process_management
 * @brief Creates and runs a new SIMIX process.
 *
 * The structure and the corresponding thread are created and put in the list of ready processes.
 *
 * @param name a name for the process. It is for user-level information and can be nullptr.
 * @param code the main function of the process
 * @param data a pointer to any data one may want to attach to the new object. It is for user-level information and can
 * be nullptr.
 * It can be retrieved with the method ActorImpl::getUserData().
 * @param host where the new agent is executed.
 * @param properties the properties of the process
 */
smx_actor_t simcall_process_create(std::string name, simgrid::simix::ActorCode code, void* data, sg_host_t host,
                                   std::unordered_map<std::string, std::string>* properties)
{
  smx_actor_t self = SIMIX_process_self();
  return simgrid::simix::simcall([name, code, data, host, properties, self] {
    return simgrid::kernel::actor::ActorImpl::create(name, std::move(code), data, host, properties, self).get();
  });
}

void simcall_process_set_data(smx_actor_t process, void* data)
{
  simgrid::simix::simcall([process, data] { process->set_user_data(data); });
}
