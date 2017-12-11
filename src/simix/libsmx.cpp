/* libsmx.c - public interface to simix                                       */
/* --------                                                                   */
/* These functions are the only ones that are visible from the higher levels  */
/* (most of them simply add some documentation to the generated simcall body) */
/*                                                                            */
/* This is somehow the "libc" of SimGrid                                      */

/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cmath>         /* std::isfinite() */

#include <functional>

#include "mc/mc.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/simix.hpp"
#include "simgrid/simix/blocking_simcall.hpp"
#include "smx_private.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/mc_forward.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_host_private.hpp"
#include "xbt/ex.h"
#include "xbt/functional.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix);

#include "popping_bodies.cpp"

void simcall_call(smx_actor_t actor)
{
  if (actor != simix_global->maestro_process) {
    XBT_DEBUG("Yield actor '%s' on simcall %s (%d)", actor->getCname(), SIMIX_simcall_name(actor->simcall.call),
              (int)actor->simcall.call);
    SIMIX_process_yield(actor);
  } else {
    SIMIX_simcall_handle(&actor->simcall, 0);
  }
}

/**
 * \ingroup simix_process_management
 * \brief Creates a synchro that executes some computation of an host.
 *
 * This function creates a SURF action and allocates the data necessary
 * to create the SIMIX synchro. It can raise a host_error exception if the host crashed.
 *
 * \param name Name of the execution synchro to create
 * \param flops_amount amount Computation amount (in flops)
 * \param priority computation priority
 * \param bound
 * \return A new SIMIX execution synchronization
 */
smx_activity_t simcall_execution_start(const char* name, double flops_amount, double priority, double bound,
                                       simgrid::s4u::Host* host)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(flops_amount), "flops_amount is not finite!");
  xbt_assert(std::isfinite(priority), "priority is not finite!");

  return simcall_BODY_execution_start(name, flops_amount, priority, bound, host);
}

/**
 * \ingroup simix_process_management
 * \brief Creates a synchro that may involve parallel computation on
 * several hosts and communication between them.
 *
 * \param name Name of the execution synchro to create
 * \param host_nb Number of hosts where the synchro will be executed
 * \param host_list Array (of size host_nb) of hosts where the synchro will be executed
 * \param flops_amount Array (of size host_nb) of computation amount of hosts (in bytes)
 * \param bytes_amount Array (of size host_nb * host_nb) representing the communication
 * amount between each pair of hosts
 * \param amount the SURF action amount
 * \param rate the SURF action rate
 * \param timeout timeout
 * \return A new SIMIX execution synchronization
 */
smx_activity_t simcall_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list,
                                                double* flops_amount, double* bytes_amount, double rate, double timeout)
{
  /* checking for infinite values */
  for (int i = 0 ; i < host_nb ; ++i) {
    xbt_assert(std::isfinite(flops_amount[i]), "flops_amount[%d] is not finite!", i);
    if (bytes_amount != nullptr) {
      for (int j = 0 ; j < host_nb ; ++j) {
        xbt_assert(std::isfinite(bytes_amount[i + host_nb * j]),
                   "bytes_amount[%d+%d*%d] is not finite!", i, host_nb, j);
      }
    }
  }

  xbt_assert(std::isfinite(rate), "rate is not finite!");

  return simcall_BODY_execution_parallel_start(name, host_nb, host_list, flops_amount, bytes_amount, rate, timeout);
}

/**
 * \ingroup simix_process_management
 * \brief Cancels an execution synchro.
 *
 * This functions stops the execution. It calls a surf function.
 * \param execution The execution synchro to cancel
 */
void simcall_execution_cancel(smx_activity_t execution)
{
  simgrid::kernel::activity::ExecImplPtr exec =
      boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(execution);
  if (not exec->surfAction_)
    return;
  simgrid::simix::kernelImmediate([exec] {
    XBT_DEBUG("Cancel synchro %p", exec.get());
    if (exec->surfAction_)
      exec->surfAction_->cancel();
  });
}

/**
 * \ingroup simix_process_management
 * \brief Changes the priority of an execution synchro.
 *
 * This functions changes the priority only. It calls a surf function.
 * \param execution The execution synchro
 * \param priority The new priority
 */
void simcall_execution_set_priority(smx_activity_t execution, double priority)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(priority), "priority is not finite!");
  simgrid::simix::kernelImmediate([execution, priority] {

    simgrid::kernel::activity::ExecImplPtr exec =
        boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(execution);
    if (exec->surfAction_)
      exec->surfAction_->setSharingWeight(priority);
  });
}

/**
 * \ingroup simix_process_management
 * \brief Changes the capping (the maximum CPU utilization) of an execution synchro.
 *
 * This functions changes the capping only. It calls a surf function.
 * \param execution The execution synchro
 * \param bound The new bound
 */
void simcall_execution_set_bound(smx_activity_t execution, double bound)
{
  simgrid::simix::kernelImmediate([execution, bound] {
    simgrid::kernel::activity::ExecImplPtr exec =
        boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(execution);
    if (exec->surfAction_)
      exec->surfAction_->setBound(bound);
  });
}

/**
 * \ingroup simix_host_management
 * \brief Waits for the completion of an execution synchro and destroy it.
 *
 * \param execution The execution synchro
 */
e_smx_state_t simcall_execution_wait(smx_activity_t execution)
{
  return (e_smx_state_t) simcall_BODY_execution_wait(execution);
}

e_smx_state_t simcall_execution_test(smx_activity_t execution)
{
  return (e_smx_state_t)simcall_BODY_execution_test(execution);
}

/**
 * \ingroup simix_process_management
 * \brief Kills all SIMIX processes.
 */
void simcall_process_killall(int reset_pid)
{
  simcall_BODY_process_killall(reset_pid);
}

/**
 * \ingroup simix_process_management
 * \brief Cleans up a SIMIX process.
 * \param process poor victim (must have already been killed)
 */
void simcall_process_cleanup(smx_actor_t process)
{
  simcall_BODY_process_cleanup(process);
}

void simcall_process_join(smx_actor_t process, double timeout)
{
  simcall_BODY_process_join(process, timeout);
}

/**
 * \ingroup simix_process_management
 * \brief Suspends a process.
 *
 * This function suspends the process by suspending the synchro
 * it was waiting for completion.
 *
 * \param process a SIMIX process
 */
void simcall_process_suspend(smx_actor_t process)
{
  simcall_BODY_process_suspend(process);
}

/**
 * \ingroup simix_process_management
 * \brief Returns the amount of SIMIX processes in the system
 *
 * Maestro internal process is not counted, only user code processes are
 */
int simcall_process_count()
{
  return simgrid::simix::kernelImmediate(SIMIX_process_count);
}

/**
 * \ingroup simix_process_management
 * \brief Set the user data of a #smx_actor_t.
 *
 * This functions sets the user data associated to \a process.
 * \param process SIMIX process
 * \param data User data
 */
void simcall_process_set_data(smx_actor_t process, void *data)
{
  simgrid::simix::kernelImmediate([process, data] { process->setUserData(data); });
}

/**
 * \ingroup simix_process_management
 * \brief Set the kill time of a process.
 */
void simcall_process_set_kill_time(smx_actor_t process, double kill_time)
{

  if (kill_time <= SIMIX_get_clock() || simix_global->kill_process_function == nullptr)
    return;
  XBT_DEBUG("Set kill time %f for process %s@%s", kill_time, process->getCname(), process->host->getCname());
  process->kill_timer = SIMIX_timer_set(kill_time, [process] {
    simix_global->kill_process_function(process);
    process->kill_timer=nullptr;
  });
}

/**
 * \ingroup simix_process_management
 * \brief Add an on_exit function
 * Add an on_exit function which will be executed when the process exits/is killed.
 */
XBT_PUBLIC(void) simcall_process_on_exit(smx_actor_t process, int_f_pvoid_pvoid_t fun, void *data)
{
  simcall_BODY_process_on_exit(process, fun, data);
}

/**
 * \ingroup simix_process_management
 * \brief Creates a new sleep SIMIX synchro.
 *
 * This function creates a SURF action and allocates the data necessary
 * to create the SIMIX synchro. It can raise a host_error exception if the
 * host crashed. The default SIMIX name of the synchro is "sleep".
 *
 *   \param duration Time duration of the sleep.
 *   \return A result telling whether the sleep was successful
 */
e_smx_state_t simcall_process_sleep(double duration)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(duration), "duration is not finite!");
  return (e_smx_state_t) simcall_BODY_process_sleep(duration);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_send(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                       size_t src_buff_size, int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data, double timeout)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");

  xbt_assert(mbox, "No rendez-vous point defined for send");

  if (MC_is_active() || MC_record_replay_is_active()) {
    /* the model-checker wants two separate simcalls */
    smx_activity_t comm = nullptr; /* MC needs the comm to be set to nullptr during the simcall */
    comm = simcall_comm_isend(sender, mbox, task_size, rate,
        src_buff, src_buff_size, match_fun, nullptr, copy_data_fun, data, 0);
    simcall_comm_wait(comm, timeout);
    comm = nullptr;
  }
  else {
    simcall_BODY_comm_send(sender, mbox, task_size, rate, src_buff, src_buff_size,
                         match_fun, copy_data_fun, data, timeout);
  }
}

/**
 * \ingroup simix_comm_management
 */
smx_activity_t simcall_comm_isend(smx_actor_t sender, smx_mailbox_t mbox, double task_size, double rate, void* src_buff,
                                  size_t src_buff_size,
                                  int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*clean_fun)(void*), void (*copy_data_fun)(smx_activity_t, void*, size_t),
                                  void* data, int detached)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");

  xbt_assert(mbox, "No rendez-vous point defined for isend");

  return simcall_BODY_comm_isend(sender, mbox, task_size, rate, src_buff,
                                 src_buff_size, match_fun,
                                 clean_fun, copy_data_fun, data, detached);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_recv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                       int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                       void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data, double timeout, double rate)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  xbt_assert(mbox, "No rendez-vous point defined for recv");

  if (MC_is_active() || MC_record_replay_is_active()) {
    /* the model-checker wants two separate simcalls */
    smx_activity_t comm = nullptr; /* MC needs the comm to be set to nullptr during the simcall */
    comm = simcall_comm_irecv(receiver, mbox, dst_buff, dst_buff_size,
                              match_fun, copy_data_fun, data, rate);
    simcall_comm_wait(comm, timeout);
    comm = nullptr;
  }
  else {
    simcall_BODY_comm_recv(receiver, mbox, dst_buff, dst_buff_size,
                           match_fun, copy_data_fun, data, timeout, rate);
  }
}
/**
 * \ingroup simix_comm_management
 */
smx_activity_t simcall_comm_irecv(smx_actor_t receiver, smx_mailbox_t mbox, void* dst_buff, size_t* dst_buff_size,
                                  int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*),
                                  void (*copy_data_fun)(smx_activity_t, void*, size_t), void* data, double rate)
{
  xbt_assert(mbox, "No rendez-vous point defined for irecv");

  return simcall_BODY_comm_irecv(receiver, mbox, dst_buff, dst_buff_size,
                                 match_fun, copy_data_fun, data, rate);
}

/**
 * \ingroup simix_comm_management
 */
smx_activity_t simcall_comm_iprobe(smx_mailbox_t mbox, int type,
                                   int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*), void* data)
{
  xbt_assert(mbox, "No rendez-vous point defined for iprobe");

  return simcall_BODY_comm_iprobe(mbox, type, match_fun, data);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_cancel(smx_activity_t synchro)
{
  simgrid::simix::kernelImmediate([synchro] {
    simgrid::kernel::activity::CommImplPtr comm =
        boost::static_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
    comm->cancel();
  });
}

/**
 * \ingroup simix_comm_management
 */
unsigned int simcall_comm_waitany(xbt_dynar_t comms, double timeout)
{
  return simcall_BODY_comm_waitany(comms, timeout);
}

/**
 * \ingroup simix_comm_management
 */
int simcall_comm_testany(smx_activity_t* comms, size_t count)
{
  if (count == 0)
    return -1;
  return simcall_BODY_comm_testany(comms, count);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_wait(smx_activity_t comm, double timeout)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simcall_BODY_comm_wait(comm, timeout);
}

/**
 * \brief Set the category of an synchro.
 *
 * This functions changes the category only. It calls a surf function.
 * \param synchro The execution synchro
 * \param category The tracing category
 */
void simcall_set_category(smx_activity_t synchro, const char *category)
{
  if (category == nullptr) {
    return;
  }
  simcall_BODY_set_category(synchro, category);
}

/**
 * \ingroup simix_comm_management
 *
 */
int simcall_comm_test(smx_activity_t comm)
{
  return simcall_BODY_comm_test(comm);
}

/**
 * \ingroup simix_synchro_management
 *
 */
smx_mutex_t simcall_mutex_init()
{
  if (not simix_global) {
    fprintf(stderr,"You must run MSG_init before using MSG\n"); // We can't use xbt_die since we may get there before the initialization
    xbt_abort();
  }
  return simgrid::simix::kernelImmediate([] { return new simgrid::simix::MutexImpl(); });
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_mutex_lock(smx_mutex_t mutex)
{
  simcall_BODY_mutex_lock(mutex);
}

/**
 * \ingroup simix_synchro_management
 *
 */
int simcall_mutex_trylock(smx_mutex_t mutex)
{
  return simcall_BODY_mutex_trylock(mutex);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_mutex_unlock(smx_mutex_t mutex)
{
  simcall_BODY_mutex_unlock(mutex);
}

/**
 * \ingroup simix_synchro_management
 *
 */
smx_cond_t simcall_cond_init()
{
  return simcall_BODY_cond_init();
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_cond_signal(smx_cond_t cond)
{
  simcall_BODY_cond_signal(cond);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex)
{
  simcall_BODY_cond_wait(cond, mutex);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex, double timeout)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simcall_BODY_cond_wait_timeout(cond, mutex, timeout);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_cond_broadcast(smx_cond_t cond)
{
  simcall_BODY_cond_broadcast(cond);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_sem_acquire(smx_sem_t sem)
{
  simcall_BODY_sem_acquire(sem);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_sem_acquire_timeout(smx_sem_t sem, double timeout)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simcall_BODY_sem_acquire_timeout(sem, timeout);
}

sg_size_t simcall_storage_read(surf_storage_t st, sg_size_t size)
{
  return simcall_BODY_storage_read(st, size);
}

sg_size_t simcall_storage_write(surf_storage_t st, sg_size_t size)
{
  return simcall_BODY_storage_write(st, size);
}

void simcall_run_kernel(std::function<void()> const& code)
{
  simcall_BODY_run_kernel(&code);
}

void simcall_run_blocking(std::function<void()> const& code)
{
  simcall_BODY_run_blocking(&code);
}

int simcall_mc_random(int min, int max) {
  return simcall_BODY_mc_random(min, max);
}

/* ************************************************************************** */

/** @brief returns a printable string representing a simcall */
const char *SIMIX_simcall_name(e_smx_simcall_t kind) {
  return simcall_names[kind];
}

namespace simgrid {
namespace simix {

void unblock(smx_actor_t process)
{
  xbt_assert(SIMIX_is_maestro());
  SIMIX_simcall_answer(&process->simcall);
}

}
}
