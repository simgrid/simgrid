/* libsmx.c - public interface to simix                                       */
/* --------                                                                   */
/* These functions are the only ones that are visible from the higher levels  */
/* (most of them simply add some documentation to the generated simcall body) */
/*                                                                            */
/* This is somehow the "libc" of SimGrid                                      */

/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cmath>         /* std::isfinite() */

#include <functional>

#include "src/mc/mc_replay.h"
#include "smx_private.h"
#include "src/mc/mc_forward.hpp"
#include "xbt/ex.h"
#include "mc/mc.h"
#include "src/simix/smx_host_private.h"
#include "src/simix/smx_private.hpp"

#include <simgrid/simix.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix);

#include "popping_bodies.cpp"

void simcall_call(smx_process_t process)
{
  if (process != simix_global->maestro_process) {
    XBT_DEBUG("Yield process '%s' on simcall %s (%d)", process->name,
              SIMIX_simcall_name(process->simcall.call), (int)process->simcall.call);
    SIMIX_process_yield(process);
  } else {
    SIMIX_simcall_handle(&process->simcall, 0);
  }
}

// ***** Host simcalls
// Those functions are replaced by methods on the Host object.

/** \ingroup simix_host_management
 * \deprecated */
xbt_swag_t simcall_host_get_process_list(sg_host_t host)
{
  return host->processes();
}

/** \ingroup simix_host_management
 * \deprecated */
double simcall_host_get_current_power_peak(sg_host_t host)
{
  return host->currentPowerPeak();
}

/** \ingroup simix_host_management
 * \deprecated */
double simcall_host_get_power_peak_at(sg_host_t host, int pstate_index)
{
  return host->powerPeakAt(pstate_index);
}

/** \deprecated */
void simcall_host_get_params(sg_host_t vm, vm_params_t params)
{
  vm->parameters(params);
}

/** \deprecated */
void simcall_host_set_params(sg_host_t vm, vm_params_t params)
{
  vm->setParameters(params);
}

/** \ingroup simix_storage_management
 *  \deprecated */
xbt_dict_t simcall_host_get_mounted_storage_list(sg_host_t host)
{
  return host->mountedStoragesAsDict();
}

/** \ingroup simix_storage_management
 *  \deprecated */
xbt_dynar_t simcall_host_get_attached_storage_list(sg_host_t host)
{
  return host->attachedStorages();
}

// ***** Other simcalls

/**
 * \ingroup simix_host_management
 * \brief Returns a dict of the properties assigned to a router or AS.
 *
 * \param name The name of the router or AS
 * \return The properties
 */
xbt_dict_t simcall_asr_get_properties(const char *name)
{
  return simcall_BODY_asr_get_properties(name);
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
 * \param affinity_mask
 * \return A new SIMIX execution synchronization
 */
smx_synchro_t simcall_execution_start(const char *name,
                                    double flops_amount,
                                    double priority, double bound, unsigned long affinity_mask)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(flops_amount), "flops_amount is not finite!");
  xbt_assert(std::isfinite(priority), "priority is not finite!");

  return simcall_BODY_execution_start(name, flops_amount, priority, bound, affinity_mask);
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
 * \return A new SIMIX execution synchronization
 */
smx_synchro_t simcall_execution_parallel_start(const char *name,
                                         int host_nb,
                                         sg_host_t *host_list,
                                         double *flops_amount,
                                         double *bytes_amount,
                                         double amount,
                                         double rate)
{
  int i,j;
  /* checking for infinite values */
  for (i = 0 ; i < host_nb ; ++i) {
    xbt_assert(std::isfinite(flops_amount[i]), "flops_amount[%d] is not finite!", i);
    if (bytes_amount != NULL) {
      for (j = 0 ; j < host_nb ; ++j) {
        xbt_assert(std::isfinite(bytes_amount[i + host_nb * j]),
                   "bytes_amount[%d+%d*%d] is not finite!", i, host_nb, j);
      }
    }
  }

  xbt_assert(std::isfinite(amount), "amount is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");

  return simcall_BODY_execution_parallel_start(name, host_nb, host_list,
                                            flops_amount,
                                            bytes_amount,
                                            amount, rate);

}

/**
 * \ingroup simix_process_management
 * \brief Destroys an execution synchro.
 *
 * Destroys a synchro, freeing its memory. This function cannot be called if there are a conditional waiting for it.
 * \param execution The execution synchro to destroy
 */
void simcall_execution_destroy(smx_synchro_t execution)
{
  simcall_BODY_execution_destroy(execution);
}

/**
 * \ingroup simix_process_management
 * \brief Cancels an execution synchro.
 *
 * This functions stops the execution. It calls a surf function.
 * \param execution The execution synchro to cancel
 */
void simcall_execution_cancel(smx_synchro_t execution)
{
  simcall_BODY_execution_cancel(execution);
}

/**
 * \ingroup simix_process_management
 * \brief Returns how much of an execution synchro remains to be done.
 *
 * \param execution The execution synchro
 * \return The remaining amount
 */
double simcall_execution_get_remains(smx_synchro_t execution)
{
  return simcall_BODY_execution_get_remains(execution);
}

/**
 * \ingroup simix_process_management
 * \brief Returns the state of an execution synchro.
 *
 * \param execution The execution synchro
 * \return The state
 */
e_smx_state_t simcall_execution_get_state(smx_synchro_t execution)
{
  return simcall_BODY_execution_get_state(execution);
}

/**
 * \ingroup simix_process_management
 * \brief Changes the priority of an execution synchro.
 *
 * This functions changes the priority only. It calls a surf function.
 * \param execution The execution synchro
 * \param priority The new priority
 */
void simcall_execution_set_priority(smx_synchro_t execution, double priority)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(priority), "priority is not finite!");

  simcall_BODY_execution_set_priority(execution, priority);
}

/**
 * \ingroup simix_process_management
 * \brief Changes the capping (the maximum CPU utilization) of an execution synchro.
 *
 * This functions changes the capping only. It calls a surf function.
 * \param execution The execution synchro
 * \param bound The new bound
 */
void simcall_execution_set_bound(smx_synchro_t execution, double bound)
{
  simcall_BODY_execution_set_bound(execution, bound);
}

/**
 * \ingroup simix_process_management
 * \brief Changes the CPU affinity of an execution synchro.
 *
 * This functions changes the CPU affinity of an execution synchro. See taskset(1) on Linux.
 * \param execution The execution synchro
 * \param host Host
 * \param mask Affinity mask
 */
void simcall_execution_set_affinity(smx_synchro_t execution, sg_host_t host, unsigned long mask)
{
  simcall_BODY_execution_set_affinity(execution, host, mask);
}

/**
 * \ingroup simix_host_management
 * \brief Waits for the completion of an execution synchro and destroy it.
 *
 * \param execution The execution synchro
 */
e_smx_state_t simcall_execution_wait(smx_synchro_t execution)
{
  return (e_smx_state_t) simcall_BODY_execution_wait(execution);
}


/**
 * \ingroup simix_vm_management
 * \brief Create a VM on the given physical host.
 *
 * \param name VM name
 * \param host Physical host
 *
 * \return The host object of the VM
 */
void* simcall_vm_create(const char *name, sg_host_t phys_host)
{
  return simgrid::simix::kernel(std::bind(SIMIX_vm_create, name, phys_host));
}

/**
 * \ingroup simix_vm_management
 * \brief Start the given VM to the given physical host
 *
 * \param vm VM
 */
void simcall_vm_start(sg_host_t vm)
{
  return simgrid::simix::kernel(std::bind(SIMIX_vm_start, vm));
}

/**
 * \ingroup simix_vm_management
 * \brief Get the state of the given VM
 *
 * \param vm VM
 * \return The state of the VM
 */
int simcall_vm_get_state(sg_host_t vm)
{
  return simgrid::simix::kernel(std::bind(SIMIX_vm_get_state, vm));
}

/**
 * \ingroup simix_vm_management
 * \brief Get the name of the physical host on which the given VM runs.
 *
 * \param vm VM
 * \return The name of the physical host
 */
void *simcall_vm_get_pm(sg_host_t vm)
{
  return simgrid::simix::kernel(std::bind(SIMIX_vm_get_pm, vm));
}

void simcall_vm_set_bound(sg_host_t vm, double bound)
{
  simgrid::simix::kernel(std::bind(SIMIX_vm_set_bound, vm, bound));
}

void simcall_vm_set_affinity(sg_host_t vm, sg_host_t pm, unsigned long mask)
{
  simgrid::simix::kernel(std::bind(SIMIX_vm_set_affinity, vm, pm, mask));
}

/**
 * \ingroup simix_vm_management
 * \brief Migrate the given VM to the given physical host
 *
 * \param vm VM
 * \param host Destination physical host
 */
void simcall_vm_migrate(sg_host_t vm, sg_host_t host)
{
  return simgrid::simix::kernel(std::bind(SIMIX_vm_migrate, vm, host));
}

/**
 * \ingroup simix_vm_management
 * \brief Suspend the given VM
 *
 * \param vm VM
 */
void simcall_vm_suspend(sg_host_t vm)
{
  simcall_BODY_vm_suspend(vm);
}

/**
 * \ingroup simix_vm_management
 * \brief Resume the given VM
 *
 * \param vm VM
 */
void simcall_vm_resume(sg_host_t vm)
{
  simcall_BODY_vm_resume(vm);
}

/**
 * \ingroup simix_vm_management
 * \brief Save the given VM
 *
 * \param vm VM
 */
void simcall_vm_save(sg_host_t vm)
{
  simcall_BODY_vm_save(vm);
}

/**
 * \ingroup simix_vm_management
 * \brief Restore the given VM
 *
 * \param vm VM
 */
void simcall_vm_restore(sg_host_t vm)
{
  simcall_BODY_vm_restore(vm);
}

/**
 * \ingroup simix_vm_management
 * \brief Shutdown the given VM
 *
 * \param vm VM
 */
void simcall_vm_shutdown(sg_host_t vm)
{
  simcall_BODY_vm_shutdown(vm);
}

/**
 * \ingroup simix_vm_management
 * \brief Destroy the given VM
 *
 * \param vm VM
 */
void simcall_vm_destroy(sg_host_t vm)
{
  simgrid::simix::kernel(std::bind(SIMIX_vm_destroy, vm));
}

/**
 * \ingroup simix_vm_management
 * \brief Encompassing simcall to prevent the removal of the src or the dst node at the end of a VM migration
 *  The simcall actually invokes the following calls: 
 *     simcall_vm_set_affinity(vm, src_pm, 0); 
 *     simcall_vm_migrate(vm, dst_pm); 
 *     simcall_vm_resume(vm);
 *
 * It is called at the end of the migration_rx_fun function from msg/msg_vm.c
 *
 * \param vm VM to migrate
 * \param src_pm  Source physical host
 * \param dst_pmt Destination physical host
 */
void simcall_vm_migratefrom_resumeto(sg_host_t vm, sg_host_t src_pm, sg_host_t dst_pm)
{
  simgrid::simix::kernel(std::bind(
    SIMIX_vm_migratefrom_resumeto, vm, src_pm, dst_pm));
}

/**
 * \ingroup simix_process_management
 * \brief Creates and runs a new SIMIX process.
 *
 * The structure and the corresponding thread are created and put in the list of ready processes.
 *
 * \param name a name for the process. It is for user-level information and can be NULL.
 * \param code the main function of the process
 * \param data a pointer to any data one may want to attach to the new object. It is for user-level information and can be NULL.
 * It can be retrieved with the function \ref simcall_process_get_data.
 * \param hostname name of the host where the new agent is executed.
 * \param kill_time time when the process is killed
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \param properties the properties of the process
 * \param auto_restart either it is autorestarting or not.
 */
smx_process_t simcall_process_create(const char *name,
                              xbt_main_func_t code,
                              void *data,
                              const char *hostname,
                              double kill_time,
                              int argc, char **argv,
                              xbt_dict_t properties,
                              int auto_restart)
{
  return (smx_process_t) simcall_BODY_process_create(name, code, data, hostname,
                              kill_time, argc, argv, properties,
                              auto_restart);
}

/**
 * \ingroup simix_process_management
 * \brief Kills a SIMIX process.
 *
 * This function simply kills a  process.
 *
 * \param process poor victim
 */
void simcall_process_kill(smx_process_t process)
{
  simcall_BODY_process_kill(process);
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
void simcall_process_cleanup(smx_process_t process)
{
  simcall_BODY_process_cleanup(process);
}

/**
 * \ingroup simix_process_management
 * \brief Migrates an agent to another location.
 *
 * This function changes the value of the host on which \a process is running.
 *
 * \param process the process to migrate
 * \param dest name of the new host
 */
void simcall_process_set_host(smx_process_t process, sg_host_t dest)
{
  simcall_BODY_process_set_host(process, dest);
}

void simcall_process_join(smx_process_t process, double timeout)
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
void simcall_process_suspend(smx_process_t process)
{
  xbt_assert(process, "Invalid parameters");

  simcall_BODY_process_suspend(process);
}

/**
 * \ingroup simix_process_management
 * \brief Resumes a suspended process.
 *
 * This function resumes a suspended process by resuming the synchro
 * it was waiting for completion.
 *
 * \param process a SIMIX process
 */
void simcall_process_resume(smx_process_t process)
{
  simcall_BODY_process_resume(process);
}

/**
 * \ingroup simix_process_management
 * \brief Returns the amount of SIMIX processes in the system
 *
 * Maestro internal process is not counted, only user code processes are
 */
int simcall_process_count(void)
{
  return simgrid::simix::kernel(SIMIX_process_count);
}

/**
 * \ingroup simix_process_management
 * \brief Return the PID of a #smx_process_t.
 * \param process a SIMIX process
 * \return the PID of this process
 */
int simcall_process_get_PID(smx_process_t process)
{
  return SIMIX_process_get_PID(process);
}

/**
 * \ingroup simix_process_management
 * \brief Return the parent PID of a #smx_process_t.
 * \param process a SIMIX process
 * \return the PID of this process parenrt
 */
int simcall_process_get_PPID(smx_process_t process)
{
  return SIMIX_process_get_PPID(process);
}

/**
 * \ingroup simix_process_management
 * \brief Return the user data of a #smx_process_t.
 * \param process a SIMIX process
 * \return the user data of this process
 */
void* simcall_process_get_data(smx_process_t process)
{
  return SIMIX_process_get_data(process);
}

/**
 * \ingroup simix_process_management
 * \brief Set the user data of a #smx_process_t.
 *
 * This functions sets the user data associated to \a process.
 * \param process SIMIX process
 * \param data User data
 */
void simcall_process_set_data(smx_process_t process, void *data)
{
  simgrid::simix::kernel(std::bind(SIMIX_process_set_data, process, data));
}

static void kill_process(void* arg)
{
  simix_global->kill_process_function((smx_process_t) arg);
}

/**
 * \ingroup simix_process_management
 * \brief Set the kill time of a process.
 */
void simcall_process_set_kill_time(smx_process_t process, double kill_time)
{

  if (kill_time > SIMIX_get_clock()) {
    if (simix_global->kill_process_function) {
      XBT_DEBUG("Set kill time %f for process %s(%s)",kill_time, process->name,
          sg_host_get_name(process->host));
      process->kill_timer = SIMIX_timer_set(kill_time, kill_process, process);
    }
  }
}
/**
 * \ingroup simix_process_management
 * \brief Get the kill time of a process (or 0 if unset).
 */
double simcall_process_get_kill_time(smx_process_t process) {
  return SIMIX_timer_get_date(process->kill_timer);
}

/**
 * \ingroup simix_process_management
 * \brief Return the location on which an agent is running.
 *
 * This functions returns the sg_host_t corresponding to the location on which
 * \a process is running.
 * \param process SIMIX process
 * \return SIMIX host
 */
sg_host_t simcall_process_get_host(smx_process_t process)
{
  return SIMIX_process_get_host(process);
}

/**
 * \ingroup simix_process_management
 * \brief Return the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not and return its name.
 * \param process SIMIX process
 * \return The process name
 */
const char* simcall_process_get_name(smx_process_t process)
{
  return SIMIX_process_get_name(process);
}

/**
 * \ingroup simix_process_management
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the task on which it was waiting for the completion.
 * \param process SIMIX process
 * \return 1, if the process is suspended, else 0.
 */
int simcall_process_is_suspended(smx_process_t process)
{
  return simcall_BODY_process_is_suspended(process);
}

/**
 * \ingroup simix_process_management
 * \brief Return the properties
 *
 * This functions returns the properties associated with this process
 */
xbt_dict_t simcall_process_get_properties(smx_process_t process)
{
  return SIMIX_process_get_properties(process);
}
/**
 * \ingroup simix_process_management
 * \brief Add an on_exit function
 * Add an on_exit function which will be executed when the process exits/is killed.
 */
XBT_PUBLIC(void) simcall_process_on_exit(smx_process_t process, int_f_pvoid_pvoid_t fun, void *data)
{
  simcall_BODY_process_on_exit(process, fun, data);
}
/**
 * \ingroup simix_process_management
 * \brief Sets the process to be auto-restarted or not by SIMIX when its host comes back up.
 * Will restart the process when the host comes back up if auto_restart is set to 1.
 */

XBT_PUBLIC(void) simcall_process_auto_restart_set(smx_process_t process, int auto_restart)
{
  simcall_BODY_process_auto_restart_set(process, auto_restart);
}

/**
 * \ingroup simix_process_management
 * \brief Restarts the process, killing it and starting it again from scratch.
 */
XBT_PUBLIC(smx_process_t) simcall_process_restart(smx_process_t process)
{
  return (smx_process_t) simcall_BODY_process_restart(process);
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
 *  \ingroup simix_rdv_management
 *  \brief Creates a new rendez-vous point
 *  \param name The name of the rendez-vous point
 *  \return The created rendez-vous point
 */
smx_mailbox_t simcall_rdv_create(const char *name)
{
  return simcall_BODY_rdv_create(name);
}


/**
 *  \ingroup simix_rdv_management
 *  \brief Destroy a rendez-vous point
 *  \param rdv The rendez-vous point to destroy
 */
void simcall_rdv_destroy(smx_mailbox_t rdv)
{
  simcall_BODY_rdv_destroy(rdv);
}
/**
 *  \ingroup simix_rdv_management
 *  \brief Returns a rendez-vous point knowing its name
 */
smx_mailbox_t simcall_rdv_get_by_name(const char *name)
{
  xbt_assert(name != NULL, "Invalid parameter for simcall_rdv_get_by_name (name is NULL)");

  /* FIXME: this is a horrible loss of performance, so we hack it out by
   * skipping the simcall (for now). It works in parallel, it won't work on
   * distributed but probably we will change MSG for that. */

  return SIMIX_rdv_get_by_name(name);
}

/**
 *  \ingroup simix_rdv_management
 *  \brief Counts the number of communication synchros of a given host pending
 *         on a rendez-vous point.
 *  \param rdv The rendez-vous point
 *  \param host The host to be counted
 *  \return The number of comm synchros pending in the rdv
 */
int simcall_rdv_comm_count_by_host(smx_mailbox_t rdv, sg_host_t host)
{
  return simcall_BODY_rdv_comm_count_by_host(rdv, host);
}

/**
 *  \ingroup simix_rdv_management
 *  \brief returns the communication at the head of the rendez-vous
 *  \param rdv The rendez-vous point
 *  \return The communication or NULL if empty
 */
smx_synchro_t simcall_rdv_get_head(smx_mailbox_t rdv)
{
  return simcall_BODY_rdv_get_head(rdv);
}

void simcall_rdv_set_receiver(smx_mailbox_t rdv, smx_process_t process)
{
  simcall_BODY_rdv_set_receiver(rdv, process);
}

smx_process_t simcall_rdv_get_receiver(smx_mailbox_t rdv)
{
  return simcall_BODY_rdv_get_receiver(rdv);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_send(smx_process_t sender, smx_mailbox_t rdv, double task_size, double rate,
                         void *src_buff, size_t src_buff_size,
                         int (*match_fun)(void *, void *, smx_synchro_t),
                         void (*copy_data_fun)(smx_synchro_t, void*, size_t), void *data,
                         double timeout)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");

  xbt_assert(rdv, "No rendez-vous point defined for send");

  if (MC_is_active() || MC_record_replay_is_active()) {
    /* the model-checker wants two separate simcalls */
    smx_synchro_t comm = NULL; /* MC needs the comm to be set to NULL during the simcall */
    comm = simcall_comm_isend(sender, rdv, task_size, rate,
        src_buff, src_buff_size, match_fun, NULL, copy_data_fun, data, 0);
    simcall_comm_wait(comm, timeout);
    comm = NULL;
  }
  else {
    simcall_BODY_comm_send(sender, rdv, task_size, rate, src_buff, src_buff_size,
                         match_fun, copy_data_fun, data, timeout);
  }
}

/**
 * \ingroup simix_comm_management
 */
smx_synchro_t simcall_comm_isend(smx_process_t sender, smx_mailbox_t rdv, double task_size, double rate,
                              void *src_buff, size_t src_buff_size,
                              int (*match_fun)(void *, void *, smx_synchro_t),
                              void (*clean_fun)(void *),
                              void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                              void *data,
                              int detached)
{
  /* checking for infinite values */
  xbt_assert(std::isfinite(task_size), "task_size is not finite!");
  xbt_assert(std::isfinite(rate), "rate is not finite!");

  xbt_assert(rdv, "No rendez-vous point defined for isend");

  return simcall_BODY_comm_isend(sender, rdv, task_size, rate, src_buff,
                                 src_buff_size, match_fun,
                                 clean_fun, copy_data_fun, data, detached);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_recv(smx_process_t receiver, smx_mailbox_t rdv, void *dst_buff, size_t * dst_buff_size,
                       int (*match_fun)(void *, void *, smx_synchro_t),
                       void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                       void *data, double timeout, double rate)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  xbt_assert(rdv, "No rendez-vous point defined for recv");

  if (MC_is_active() || MC_record_replay_is_active()) {
    /* the model-checker wants two separate simcalls */
    smx_synchro_t comm = NULL; /* MC needs the comm to be set to NULL during the simcall */
    comm = simcall_comm_irecv(receiver, rdv, dst_buff, dst_buff_size,
                              match_fun, copy_data_fun, data, rate);
    simcall_comm_wait(comm, timeout);
    comm = NULL;
  }
  else {
    simcall_BODY_comm_recv(receiver, rdv, dst_buff, dst_buff_size,
                           match_fun, copy_data_fun, data, timeout, rate);
  }
}
/**
 * \ingroup simix_comm_management
 */
smx_synchro_t simcall_comm_irecv(smx_process_t receiver, smx_mailbox_t rdv, void *dst_buff, size_t *dst_buff_size,
                                int (*match_fun)(void *, void *, smx_synchro_t),
                                void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                                void *data, double rate)
{
  xbt_assert(rdv, "No rendez-vous point defined for irecv");

  return simcall_BODY_comm_irecv(receiver, rdv, dst_buff, dst_buff_size,
                                 match_fun, copy_data_fun, data, rate);
}

/**
 * \ingroup simix_comm_management
 */
smx_synchro_t simcall_comm_iprobe(smx_mailbox_t rdv, int type, int src, int tag,
                                int (*match_fun)(void *, void *, smx_synchro_t), void *data)
{
  xbt_assert(rdv, "No rendez-vous point defined for iprobe");

  return simcall_BODY_comm_iprobe(rdv, type, src, tag, match_fun, data);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_cancel(smx_synchro_t comm)
{
  simcall_BODY_comm_cancel(comm);
}

/**
 * \ingroup simix_comm_management
 */
unsigned int simcall_comm_waitany(xbt_dynar_t comms)
{
  return simcall_BODY_comm_waitany(comms);
}

/**
 * \ingroup simix_comm_management
 */
int simcall_comm_testany(xbt_dynar_t comms)
{
  if (xbt_dynar_is_empty(comms))
    return -1;
  return simcall_BODY_comm_testany(comms);
}

/**
 * \ingroup simix_comm_management
 */
void simcall_comm_wait(smx_synchro_t comm, double timeout)
{
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simcall_BODY_comm_wait(comm, timeout);
}

/**
 * \brief Set the category of an synchro.
 *
 * This functions changes the category only. It calls a surf function.
 * \param execution The execution synchro
 * \param category The tracing category
 */
void simcall_set_category(smx_synchro_t synchro, const char *category)
{
  if (category == NULL) {
    return;
  }
  simcall_BODY_set_category(synchro, category);
}

/**
 * \ingroup simix_comm_management
 *
 */
int simcall_comm_test(smx_synchro_t comm)
{
  return simcall_BODY_comm_test(comm);
}

/**
 * \ingroup simix_comm_management
 *
 */
double simcall_comm_get_remains(smx_synchro_t comm)
{
  return simcall_BODY_comm_get_remains(comm);
}

/**
 * \ingroup simix_comm_management
 *
 */
e_smx_state_t simcall_comm_get_state(smx_synchro_t comm)
{
  return simcall_BODY_comm_get_state(comm);
}

/**
 * \ingroup simix_comm_management
 *
 */
void *simcall_comm_get_src_data(smx_synchro_t comm)
{
  return simcall_BODY_comm_get_src_data(comm);
}

/**
 * \ingroup simix_comm_management
 *
 */
void *simcall_comm_get_dst_data(smx_synchro_t comm)
{
  return simcall_BODY_comm_get_dst_data(comm);
}

/**
 * \ingroup simix_comm_management
 *
 */
smx_process_t simcall_comm_get_src_proc(smx_synchro_t comm)
{
  return simcall_BODY_comm_get_src_proc(comm);
}

/**
 * \ingroup simix_comm_management
 *
 */
smx_process_t simcall_comm_get_dst_proc(smx_synchro_t comm)
{
  return simcall_BODY_comm_get_dst_proc(comm);
}

/**
 * \ingroup simix_synchro_management
 *
 */
smx_mutex_t simcall_mutex_init(void)
{
  if(!simix_global) {
    fprintf(stderr,"You must run MSG_init before using MSG\n"); // We can't use xbt_die since we may get there before the initialization
    xbt_abort();
  }
  return simcall_BODY_mutex_init();
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
smx_cond_t simcall_cond_init(void)
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
void simcall_cond_wait_timeout(smx_cond_t cond,
                                 smx_mutex_t mutex,
                                 double timeout)
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
smx_sem_t simcall_sem_init(int capacity)
{
  return simcall_BODY_sem_init(capacity);
}

/**
 * \ingroup simix_synchro_management
 *
 */
void simcall_sem_release(smx_sem_t sem)
{
  simcall_BODY_sem_release(sem);
}

/**
 * \ingroup simix_synchro_management
 *
 */
int simcall_sem_would_block(smx_sem_t sem)
{
  return simcall_BODY_sem_would_block(sem);
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

/**
 * \ingroup simix_synchro_management
 *
 */
int simcall_sem_get_capacity(smx_sem_t sem)
{
  return simcall_BODY_sem_get_capacity(sem);
}

/**
 * \ingroup simix_file_management
 *
 */
sg_size_t simcall_file_read(smx_file_t fd, sg_size_t size, sg_host_t host)
{
  return simcall_BODY_file_read(fd, size, host);
}

/**
 * \ingroup simix_file_management
 *
 */
sg_size_t simcall_file_write(smx_file_t fd, sg_size_t size, sg_host_t host)
{
  return simcall_BODY_file_write(fd, size, host);
}

/**
 * \ingroup simix_file_management
 * \brief
 */
smx_file_t simcall_file_open(const char* fullpath, sg_host_t host)
{
  return simcall_BODY_file_open(fullpath, host);
}

/**
 * \ingroup simix_file_management
 *
 */
int simcall_file_close(smx_file_t fd, sg_host_t host)
{
  return simcall_BODY_file_close(fd, host);
}

/**
 * \ingroup simix_file_management
 *
 */
int simcall_file_unlink(smx_file_t fd, sg_host_t host)
{
  return simcall_BODY_file_unlink(fd, host);
}

/**
 * \ingroup simix_file_management
 *
 */
sg_size_t simcall_file_get_size(smx_file_t fd){
  return simcall_BODY_file_get_size(fd);
}

/**
 * \ingroup simix_file_management
 *
 */
sg_size_t simcall_file_tell(smx_file_t fd){
  return simcall_BODY_file_tell(fd);
}

/**
 * \ingroup simix_file_management
 *
 */
xbt_dynar_t simcall_file_get_info(smx_file_t fd)
{
  return simcall_BODY_file_get_info(fd);
}

/**
 * \ingroup simix_file_management
 *
 */
int simcall_file_seek(smx_file_t fd, sg_offset_t offset, int origin){
  return simcall_BODY_file_seek(fd, offset, origin);
}

/**
 * \ingroup simix_file_management
 * \brief Move a file to another location on the *same mount point*.
 *
 */
int simcall_file_move(smx_file_t fd, const char* fullpath)
{
  return simcall_BODY_file_move(fd, fullpath);
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the free space size on a given storage element.
 * \param storage a storage
 * \return Return the free space size on a given storage element (as sg_size_t)
 */
sg_size_t simcall_storage_get_free_size (smx_storage_t storage){
  return simcall_BODY_storage_get_free_size(storage);
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the used space size on a given storage element.
 * \param storage a storage
 * \return Return the used space size on a given storage element (as sg_size_t)
 */
sg_size_t simcall_storage_get_used_size (smx_storage_t storage){
  return simcall_BODY_storage_get_used_size(storage);
}

/**
 * \ingroup simix_storage_management
 * \brief Returns a dict of the properties assigned to a storage element.
 *
 * \param storage A storage element
 * \return The properties of this storage element
 */
xbt_dict_t simcall_storage_get_properties(smx_storage_t storage)
{
  return simcall_BODY_storage_get_properties(storage);
}

/**
 * \ingroup simix_storage_management
 * \brief Returns a dict containing the content of a storage element.
 *
 * \param storage A storage element
 * \return The content of this storage element as a dict (full path file => size)
 */
xbt_dict_t simcall_storage_get_content(smx_storage_t storage)
{
  return simcall_BODY_storage_get_content(storage);
}

void simcall_run_kernel(std::function<void()> const& code)
{
  return simcall_BODY_run_kernel((void*) &code);
}

int simcall_mc_random(int min, int max) {
  return simcall_BODY_mc_random(min, max);
}

/* ************************************************************************** */

/** @brief returns a printable string representing a simcall */
const char *SIMIX_simcall_name(e_smx_simcall_t kind) {
  return simcall_names[kind];
}
