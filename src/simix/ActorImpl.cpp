/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <exception>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include <boost/range/algorithm.hpp>

#include "xbt/ex.hpp"
#include "xbt/functional.hpp"
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "xbt/utility.hpp"

#include "simgrid/s4u/Host.hpp"

#include "mc/mc.h"

#include "smx_private.hpp"
#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/activity/SynchroIo.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/mc/remote/Client.hpp"
#include "src/msg/msg_private.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"

#ifdef HAVE_SMPI
#include "src/smpi/include/private.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix, "Logging specific to SIMIX (process)");

unsigned long simix_process_maxpid = 0;

/**
 * \brief Returns the current agent.
 *
 * This functions returns the currently running SIMIX process.
 *
 * \return The SIMIX process
 */
smx_actor_t SIMIX_process_self()
{
  smx_context_t self_context = SIMIX_context_self();

  return (self_context != nullptr) ? self_context->process() : nullptr;
}

/**
 * \brief Returns whether a process has pending asynchronous communications.
 * \return true if there are asynchronous communications in this process
 */
int SIMIX_process_has_pending_comms(smx_actor_t process) {

  return process->comms.size() > 0;
}

/**
 * \brief Moves a process to the list of processes to destroy.
 */
void SIMIX_process_cleanup(smx_actor_t process)
{
  XBT_DEBUG("Cleanup process %s (%p), waiting synchro %p", process->name.c_str(), process,
            process->waiting_synchro.get());

  process->finished = true;
  SIMIX_process_on_exit_runall(process);

  /* Unregister from the kill timer if any */
  if (process->kill_timer != nullptr) {
    SIMIX_timer_remove(process->kill_timer);
    process->kill_timer = nullptr;
  }

  xbt_os_mutex_acquire(simix_global->mutex);

  /* cancel non-blocking communications */
  while (not process->comms.empty()) {
    smx_activity_t synchro = process->comms.front();
    process->comms.pop_front();
    simgrid::kernel::activity::CommImplPtr comm =
        boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);

    /* make sure no one will finish the comm after this process is destroyed,
     * because src_proc or dst_proc would be an invalid pointer */

    if (comm->src_proc == process) {
      XBT_DEBUG("Found an unfinished send comm %p (detached = %d), state %d, src = %p, dst = %p", comm.get(),
                comm->detached, (int)comm->state, comm->src_proc, comm->dst_proc);
      comm->src_proc = nullptr;

    } else if (comm->dst_proc == process) {
      XBT_DEBUG("Found an unfinished recv comm %p, state %d, src = %p, dst = %p", comm.get(), (int)comm->state,
                comm->src_proc, comm->dst_proc);
      comm->dst_proc = nullptr;

      if (comm->detached && comm->src_proc != nullptr) {
        /* the comm will be freed right now, remove it from the sender */
        comm->src_proc->comms.remove(comm);
      }
    } else {
      xbt_die("Communication synchro %p is in my list but I'm not the sender nor the receiver", synchro.get());
    }
    comm->cancel();
  }

  XBT_DEBUG("%p should not be run anymore",process);
  simix_global->process_list.erase(process->pid);
  if (process->host && process->host_process_list_hook.is_linked())
    simgrid::xbt::intrusive_erase(process->host->extension<simgrid::simix::Host>()->process_list, *process);
  if (not process->smx_destroy_list_hook.is_linked()) {
#if SIMGRID_HAVE_MC
    xbt_dynar_push_as(simix_global->dead_actors_vector, smx_actor_t, process);
#endif
    simix_global->process_to_destroy.push_back(*process);
  }
  process->context->iwannadie = 0;

  xbt_os_mutex_release(simix_global->mutex);
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
namespace simix {

ActorImpl::~ActorImpl()
{
  delete this->context;
}

static int dying_daemon(void* exit_status, void* data)
{
  std::vector<ActorImpl*>* vect = &simix_global->daemons;

  auto it = std::find(vect->begin(), vect->end(), static_cast<ActorImpl*>(data));
  xbt_assert(it != vect->end(), "The dying daemon is not a daemon after all. Please report that bug.");

  /* Don't move the whole content since we don't really care about the order */
  std::swap(*it, vect->back());
  vect->pop_back();

  return 0;
}
/** This process will be terminated automatically when the last non-daemon process finishes */
void ActorImpl::daemonize()
{
  if (not daemon) {
    daemon = true;
    simix_global->daemons.push_back(this);
    SIMIX_process_on_exit(this, dying_daemon, this);
  }
}

simgrid::s4u::Actor* ActorImpl::restart()
{
  XBT_DEBUG("Restarting process %s on %s", getCname(), host->getCname());

  // retrieve the arguments of the old process
  // FIXME: Factorize this with SIMIX_host_add_auto_restart_process ?
  simgrid::simix::ProcessArg arg;
  arg.name         = name;
  arg.code         = code;
  arg.host         = host;
  arg.kill_time    = SIMIX_timer_get_date(kill_timer);
  arg.data         = userdata;
  arg.properties   = nullptr;
  arg.auto_restart = auto_restart;

  // kill the old process
  SIMIX_process_kill(this, (this == simix_global->maestro_process) ? this : SIMIX_process_self());

  // start the new process
  ActorImpl* actor = simix_global->create_process_function(arg.name.c_str(), std::move(arg.code), arg.data, arg.host,
                                                           arg.properties.get(), nullptr);
  if (arg.kill_time >= 0)
    simcall_process_set_kill_time(actor, arg.kill_time);
  if (arg.auto_restart)
    actor->auto_restart = arg.auto_restart;

  return actor->ciface();
}

smx_activity_t ActorImpl::suspend(ActorImpl* issuer)
{
  if (suspended) {
    XBT_DEBUG("Actor '%s' is already suspended", name.c_str());
    return nullptr;
  }

  suspended = 1;

  /* If we are suspending another actor that is waiting on a sync, suspend its synchronization. */
  if (this != issuer) {
    if (waiting_synchro)
      waiting_synchro->suspend();
    /* If the other actor is not waiting, its suspension is delayed to when the actor is rescheduled. */

    return nullptr;
  } else {
    return SIMIX_execution_start("suspend", 0.0, 1.0, 0.0, this->host);
  }
}

void ActorImpl::resume()
{
  XBT_IN("process = %p", this);

  if (context->iwannadie) {
    XBT_VERB("Ignoring request to suspend an actor that is currently dying.");
    return;
  }

  if (not suspended)
    return;
  suspended = 0;

  /* resume the synchronization that was blocking the resumed actor. */
  if (waiting_synchro)
    waiting_synchro->resume();

  XBT_OUT();
}

smx_activity_t ActorImpl::sleep(double duration)
{
  if (host->isOff())
    THROWF(host_error, 0, "Host %s failed, you cannot sleep there.", host->getCname());

  simgrid::kernel::activity::SleepImpl* synchro = new simgrid::kernel::activity::SleepImpl();
  synchro->host                                 = host;
  synchro->surf_sleep                           = host->pimpl_cpu->sleep(duration);
  synchro->surf_sleep->setData(synchro);
  XBT_DEBUG("Create sleep synchronization %p", synchro);

  return synchro;
}

void create_maestro(std::function<void()> code)
{
  smx_actor_t maestro = nullptr;
  /* Create maestro process and initialize it */
  maestro = new simgrid::simix::ActorImpl();
  maestro->pid = simix_process_maxpid++;
  maestro->name = "";
  maestro->userdata = nullptr;

  if (not code) {
    maestro->context = SIMIX_context_new(std::function<void()>(), nullptr, maestro);
  } else {
    if (not simix_global)
      xbt_die("simix is not initialized, please call MSG_init first");
    maestro->context = simix_global->context_factory->create_maestro(code, maestro);
  }

  maestro->simcall.issuer = maestro;
  simix_global->maestro_process = maestro;
}

}
}

/** @brief Creates and runs the maestro process */
void SIMIX_maestro_create(void (*code)(void*), void* data)
{
  simgrid::simix::create_maestro(std::bind(code, data));
}

/**
 * \brief Internal function to create a process.
 *
 * This function actually creates the process.
 * It may be called when a SIMCALL_PROCESS_CREATE simcall occurs,
 * or directly for SIMIX internal purposes. The sure thing is that it's called from maestro context.
 *
 * \return the process created
 */
smx_actor_t SIMIX_process_create(const char* name, std::function<void()> code, void* data, simgrid::s4u::Host* host,
                                 std::map<std::string, std::string>* properties, smx_actor_t parent_process)
{

  XBT_DEBUG("Start process %s on host '%s'", name, host->getCname());

  if (host->isOff()) {
    XBT_WARN("Cannot launch process '%s' on failed host '%s'", name, host->getCname());
    return nullptr;
  }

  smx_actor_t process = new simgrid::simix::ActorImpl();

  xbt_assert(code && host != nullptr, "Invalid parameters");
  /* Process data */
  process->pid            = simix_process_maxpid++;
  process->name           = simgrid::xbt::string(name);
  process->host           = host;
  process->userdata       = data;
  process->simcall.issuer = process;

  if (parent_process != nullptr) {
    process->ppid = parent_process->pid;
/* SMPI process have their own data segment and each other inherit from their father */
#if HAVE_SMPI
    if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) {
      if (parent_process->pid != 0) {
        process->segment_index = parent_process->segment_index;
      } else {
        process->segment_index = process->pid - 1;
      }
    }
#endif
  }

  process->code         = code;

  XBT_VERB("Create context %s", process->name.c_str());
  process->context = SIMIX_context_new(std::move(code), simix_global->cleanup_process_function, process);

  /* Add properties */
  if (properties != nullptr)
    for (auto const& kv : *properties)
      process->setProperty(kv.first, kv.second);

  /* Make sure that the process is initialized for simix, in case we are called from the Host::onCreation signal */
  if (host->extension<simgrid::simix::Host>() == nullptr)
    host->extension_set<simgrid::simix::Host>(new simgrid::simix::Host());

  /* Add the process to its host process list */
  host->extension<simgrid::simix::Host>()->process_list.push_back(*process);

  XBT_DEBUG("Start context '%s'", process->name.c_str());

  /* Now insert it in the global process list and in the process to run list */
  simix_global->process_list[process->pid] = process;
  XBT_DEBUG("Inserting %s(%s) in the to_run list", process->getCname(), host->getCname());
  simix_global->process_to_run.push_back(process);
  intrusive_ptr_add_ref(process);

  /* Tracing the process creation */
  TRACE_msg_process_create(process->getName(), process->pid, process->host);

  return process;
}

smx_actor_t SIMIX_process_attach(const char* name, void* data, const char* hostname,
                                 std::map<std::string, std::string>* properties, smx_actor_t parent_process)
{
  // This is mostly a copy/paste from SIMIX_process_new(),
  // it'd be nice to share some code between those two functions.

  sg_host_t host = sg_host_by_name(hostname);
  XBT_DEBUG("Attach process %s on host '%s'", name, hostname);

  if (host->isOff()) {
    XBT_WARN("Cannot launch process '%s' on failed host '%s'", name, hostname);
    return nullptr;
  }

  smx_actor_t process = new simgrid::simix::ActorImpl();
  /* Process data */
  process->pid = simix_process_maxpid++;
  process->name = std::string(name);
  process->host = host;
  process->userdata       = data;
  process->simcall.issuer = process;

  if (parent_process != nullptr) {
    process->ppid = parent_process->pid;
    /* SMPI process have their own data segment and each other inherit from their father */
#if HAVE_SMPI
    if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) {
      if (parent_process->pid != 0) {
        process->segment_index = parent_process->segment_index;
      } else {
        process->segment_index = process->pid - 1;
      }
    }
#endif
  }

  /* Process data for auto-restart */
  process->code = nullptr;

  XBT_VERB("Create context %s", process->name.c_str());
  if (not simix_global)
    xbt_die("simix is not initialized, please call MSG_init first");
  process->context = simix_global->context_factory->attach(simix_global->cleanup_process_function, process);

  /* Add properties */
  if (properties != nullptr)
    for (auto const& kv : *properties)
      process->setProperty(kv.first, kv.second);

  /* Add the process to it's host process list */
  host->extension<simgrid::simix::Host>()->process_list.push_back(*process);

  /* Now insert it in the global process list and in the process to run list */
  simix_global->process_list[process->pid] = process;
  XBT_DEBUG("Inserting %s(%s) in the to_run list", process->getCname(), host->getCname());
  simix_global->process_to_run.push_back(process);

  /* Tracing the process creation */
  TRACE_msg_process_create(process->getName(), process->pid, process->host);

  auto context = dynamic_cast<simgrid::kernel::context::AttachContext*>(process->context);
  if (not context)
    xbt_die("Not a suitable context");

  context->attach_start();
  return process;
}

void SIMIX_process_detach()
{
  auto context = dynamic_cast<simgrid::kernel::context::AttachContext*>(SIMIX_context_self());
  if (not context)
    xbt_die("Not a suitable context");

  auto process = context->process();
  simix_global->cleanup_process_function(process);
  context->attach_stop();
  delete process;
}

/**
 * \brief Executes the processes from simix_global->process_to_run.
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
 * \brief Internal function to kill a SIMIX process.
 *
 * This function may be called when a SIMCALL_PROCESS_KILL simcall occurs,
 * or directly for SIMIX internal purposes.
 *
 * \param process poor victim
 * \param issuer the process which has sent the PROCESS_KILL. Important to not schedule twice the same process.
 */
void SIMIX_process_kill(smx_actor_t process, smx_actor_t issuer) {

  if (process->finished) {
    XBT_DEBUG("Ignoring request to kill process %s@%s that is already dead", process->getCname(),
              process->host->getCname());
    return;
  }

  XBT_DEBUG("Killing process %s@%s", process->getCname(), process->host->getCname());

  process->context->iwannadie = 1;
  process->blocked = 0;
  process->suspended = 0;
  process->exception = nullptr;

  /* destroy the blocking synchro if any */
  if (process->waiting_synchro != nullptr) {

    simgrid::kernel::activity::ExecImplPtr exec =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(process->waiting_synchro);
    simgrid::kernel::activity::CommImplPtr comm =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(process->waiting_synchro);
    simgrid::kernel::activity::SleepImplPtr sleep =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(process->waiting_synchro);
    simgrid::kernel::activity::RawImplPtr raw =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::RawImpl>(process->waiting_synchro);
    simgrid::kernel::activity::IoImplPtr io =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::IoImpl>(process->waiting_synchro);

    if (exec != nullptr) {
      if (exec->surfAction_) {
        exec->surfAction_->cancel();
        exec->surfAction_->unref();
        exec->surfAction_ = nullptr;
      }
    } else if (comm != nullptr) {
      process->comms.remove(process->waiting_synchro);
      comm->cancel();
      // Remove first occurrence of &process->simcall:
      auto i = boost::range::find(process->waiting_synchro->simcalls, &process->simcall);
      if (i != process->waiting_synchro->simcalls.end())
        process->waiting_synchro->simcalls.remove(&process->simcall);
    } else if (sleep != nullptr) {
      SIMIX_process_sleep_destroy(process->waiting_synchro);

    } else if (raw != nullptr) {
      SIMIX_synchro_stop_waiting(process, &process->simcall);

    } else if (io != nullptr) {
      SIMIX_io_destroy(process->waiting_synchro);
    } else {
      xbt_die("Unknown type of activity");
    }

    process->waiting_synchro = nullptr;
  }
  if (std::find(begin(simix_global->process_to_run), end(simix_global->process_to_run), process) ==
          end(simix_global->process_to_run) &&
      process != issuer) {
    XBT_DEBUG("Inserting %s in the to_run list", process->name.c_str());
    simix_global->process_to_run.push_back(process);
  }
}

/** @brief Ask another process to raise the given exception
 *
 * @param process The process that should raise that exception
 * @param cat category of exception
 * @param value value associated to the exception
 * @param msg string information associated to the exception
 */
void SIMIX_process_throw(smx_actor_t process, xbt_errcat_t cat, int value, const char *msg) {
  SMX_EXCEPTION(process, cat, value, msg);

  if (process->suspended)
    process->resume();

  /* cancel the blocking synchro if any */
  if (process->waiting_synchro) {

    simgrid::kernel::activity::ExecImplPtr exec =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(process->waiting_synchro);
    if (exec != nullptr && exec->surfAction_)
      exec->surfAction_->cancel();

    simgrid::kernel::activity::CommImplPtr comm =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(process->waiting_synchro);
    if (comm != nullptr) {
      process->comms.remove(comm);
      comm->cancel();
    }

    simgrid::kernel::activity::SleepImplPtr sleep =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(process->waiting_synchro);
    if (sleep != nullptr) {
      SIMIX_process_sleep_destroy(process->waiting_synchro);
      if (std::find(begin(simix_global->process_to_run), end(simix_global->process_to_run), process) ==
              end(simix_global->process_to_run) &&
          process != SIMIX_process_self()) {
        XBT_DEBUG("Inserting %s in the to_run list", process->name.c_str());
        simix_global->process_to_run.push_back(process);
      }
    }

    simgrid::kernel::activity::RawImplPtr raw =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::RawImpl>(process->waiting_synchro);
    if (raw != nullptr) {
      SIMIX_synchro_stop_waiting(process, &process->simcall);
    }

    simgrid::kernel::activity::IoImplPtr io =
        boost::dynamic_pointer_cast<simgrid::kernel::activity::IoImpl>(process->waiting_synchro);
    if (io != nullptr) {
      SIMIX_io_destroy(process->waiting_synchro);
    }
  }
  process->waiting_synchro = nullptr;

}

void simcall_HANDLER_process_killall(smx_simcall_t simcall, int reset_pid) {
  SIMIX_process_killall(simcall->issuer, reset_pid);
}
/**
 * \brief Kills all running processes.
 * \param issuer this one will not be killed
 */
void SIMIX_process_killall(smx_actor_t issuer, int reset_pid)
{
  for (auto const& kv : simix_global->process_list)
    if (kv.second != issuer)
      SIMIX_process_kill(kv.second, issuer);

  if (reset_pid > 0)
    simix_process_maxpid = reset_pid;
}

void SIMIX_process_change_host(smx_actor_t process, sg_host_t dest)
{
  xbt_assert((process != nullptr), "Invalid parameters");
  simgrid::xbt::intrusive_erase(process->host->extension<simgrid::simix::Host>()->process_list, *process);
  process->host = dest;
  dest->extension<simgrid::simix::Host>()->process_list.push_back(*process);
}

void simcall_HANDLER_process_suspend(smx_simcall_t simcall, smx_actor_t process)
{
  smx_activity_t sync_suspend = process->suspend(simcall->issuer);

  if (process != simcall->issuer) {
    SIMIX_simcall_answer(simcall);
  } else {
    sync_suspend->simcalls.push_back(simcall);
    process->waiting_synchro = sync_suspend;
    process->waiting_synchro->suspend();
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

  if (not self) {
    return nullptr;
  }
  return self->getUserData();
}

void SIMIX_process_self_set_data(void *data)
{
  SIMIX_process_self()->setUserData(data);
}


/* needs to be public and without simcall because it is called
   by exceptions and logging events */
const char* SIMIX_process_self_get_name() {

  smx_actor_t process = SIMIX_process_self();
  if (process == nullptr || process == simix_global->maestro_process)
    return "maestro";

  return process->name.c_str();
}

smx_actor_t SIMIX_process_get_by_name(const char* name)
{
  for (auto const& kv : simix_global->process_list)
    if (kv.second->name == name)
      return kv.second;
  return nullptr;
}

void simcall_HANDLER_process_join(smx_simcall_t simcall, smx_actor_t process, double timeout)
{
  if (process->finished) {
    // The joined process is already finished, just wake up the issuer process right away
    simcall_process_sleep__set__result(simcall, SIMIX_DONE);
    SIMIX_simcall_answer(simcall);
    return;
  }
  smx_activity_t sync = SIMIX_process_join(simcall->issuer, process, timeout);
  sync->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = sync;
}

smx_activity_t SIMIX_process_join(smx_actor_t issuer, smx_actor_t process, double timeout)
{
  smx_activity_t res = issuer->sleep(timeout);
  intrusive_ptr_add_ref(res.get());
  SIMIX_process_on_exit(process,
                        [](void*, void* arg) {
                          auto sleep = static_cast<simgrid::kernel::activity::SleepImpl*>(arg);
                          if (sleep->surf_sleep)
                            sleep->surf_sleep->finish(simgrid::surf::Action::State::done);
                          intrusive_ptr_release(sleep);
                          return 0;
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
  sync->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = sync;
}

void SIMIX_process_sleep_destroy(smx_activity_t synchro)
{
  XBT_DEBUG("Destroy sleep synchro %p", synchro.get());
  simgrid::kernel::activity::SleepImplPtr sleep =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::SleepImpl>(synchro);

  if (sleep->surf_sleep) {
    sleep->surf_sleep->unref();
    sleep->surf_sleep = nullptr;
  }
}

/**
 * \brief Calling this function makes the process to yield.
 *
 * Only the current process can call this function, giving back the control to maestro.
 *
 * \param self the current process
 */
void SIMIX_process_yield(smx_actor_t self)
{
  XBT_DEBUG("Yield actor '%s'", self->getCname());

  /* Go into sleep and return control to maestro */
  self->context->suspend();

  /* Ok, maestro returned control to us */
  XBT_DEBUG("Control returned to me: '%s'", self->name.c_str());

  if (self->new_host) {
    SIMIX_process_change_host(self, self->new_host);
    self->new_host = nullptr;
  }

  if (self->context->iwannadie){
    XBT_DEBUG("I wanna die!");
    self->finished = true;
    /* execute the on_exit functions */
    SIMIX_process_on_exit_runall(self);
    /* Add the process to the list of process to restart, only if the host is down */
    if (self->auto_restart && self->host->isOff()) {
      SIMIX_host_add_auto_restart_process(self->host, self->getCname(), self->code, self->userdata,
                                          SIMIX_timer_get_date(self->kill_timer), self->getProperties(),
                                          self->auto_restart);
    }
    XBT_DEBUG("Process %s@%s is dead", self->getCname(), self->host->getCname());
    self->context->stop();
  }

  if (self->suspended) {
    XBT_DEBUG("Hey! I'm suspended.");
    xbt_assert(self->exception != nullptr, "Gasp! This exception may be lost by subsequent calls.");
    self->suspended = 0;
    self->suspend(self);
  }

  if (self->exception != nullptr) {
    XBT_DEBUG("Wait, maestro left me an exception");
    std::exception_ptr exception = std::move(self->exception);
    self->exception = nullptr;
    std::rethrow_exception(std::move(exception));
  }

  if(SMPI_switch_data_segment && self->segment_index != -1){
    SMPI_switch_data_segment(self->segment_index);
  }
}

/* callback: termination */
void SIMIX_process_exception_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  xbt_abort();
}

/** @brief Returns the list of processes to run. */
const std::vector<smx_actor_t>& simgrid::simix::process_get_runnable()
{
  return simix_global->process_to_run;
}

/** @brief Returns the process from PID. */
smx_actor_t SIMIX_process_from_PID(aid_t PID)
{
  auto process = simix_global->process_list.find(PID);
  return process == simix_global->process_list.end() ? nullptr : process->second;
}

void SIMIX_process_on_exit_runall(smx_actor_t process) {
  smx_process_exit_status_t exit_status = (process->context->iwannadie) ? SMX_EXIT_FAILURE : SMX_EXIT_SUCCESS;
  while (not process->on_exit.empty()) {
    s_smx_process_exit_fun_t exit_fun = process->on_exit.back();
    process->on_exit.pop_back();
    (exit_fun.fun)((void*)exit_status, exit_fun.arg);
  }
}

void SIMIX_process_on_exit(smx_actor_t process, int_f_pvoid_pvoid_t fun, void *data) {
  xbt_assert(process, "current process not found: are you in maestro context ?");

  s_smx_process_exit_fun_t exit_fun = {fun, data};

  process->on_exit.push_back(exit_fun);
}

/**
 * \brief Sets the auto-restart status of the process.
 * If set to 1, the process will be automatically restarted when its host comes back.
 */
void SIMIX_process_auto_restart_set(smx_actor_t process, int auto_restart) {
  process->auto_restart = auto_restart;
}

/** @brief Restart a process, starting it again from the beginning. */
/**
 * \ingroup simix_process_management
 * \brief Creates and runs a new SIMIX process.
 *
 * The structure and the corresponding thread are created and put in the list of ready processes.
 *
 * \param name a name for the process. It is for user-level information and can be nullptr.
 * \param code the main function of the process
 * \param data a pointer to any data one may want to attach to the new object. It is for user-level information and can
 * be nullptr.
 * It can be retrieved with the function \ref simcall_process_get_data.
 * \param host where the new agent is executed.
 * \param kill_time time when the process is killed
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \param properties the properties of the process
 * \param auto_restart either it is autorestarting or not.
 */
extern "C" smx_actor_t simcall_process_create(const char* name, xbt_main_func_t code, void* data, sg_host_t host,
                                              int argc, char** argv, std::map<std::string, std::string>* properties)
{
  if (name == nullptr)
    name = "";
  auto wrapped_code = simgrid::xbt::wrapMain(code, argc, argv);
  for (int i = 0; i != argc; ++i)
    xbt_free(argv[i]);
  xbt_free(argv);
  smx_actor_t res = simcall_process_create(name, std::move(wrapped_code), data, host, properties);
  return res;
}

smx_actor_t simcall_process_create(const char* name, std::function<void()> code, void* data, sg_host_t host,
                                   std::map<std::string, std::string>* properties)
{
  if (name == nullptr)
    name = "";
  smx_actor_t self = SIMIX_process_self();
  return simgrid::simix::kernelImmediate([name, code, data, host, properties, self] {
    return SIMIX_process_create(name, std::move(code), data, host, properties, self);
  });
}
