/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/ConditionVariableImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "src/surf/cpu_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix, "SIMIX Synchronization (mutex, semaphores and conditions)");

/***************************** Raw synchronization *********************************/

smx_activity_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout)
{
  XBT_IN("(%p, %f)",smx_host,timeout);

  simgrid::kernel::activity::RawImplPtr sync =
      simgrid::kernel::activity::RawImplPtr(new simgrid::kernel::activity::RawImpl());
  sync->surf_action_ = smx_host->pimpl_cpu->sleep(timeout);
  sync->surf_action_->set_data(sync.get());
  XBT_OUT();
  return sync;
}

void SIMIX_synchro_stop_waiting(smx_actor_t process, smx_simcall_t simcall)
{
  XBT_IN("(%p, %p)",process,simcall);
  switch (simcall->call) {

    case SIMCALL_MUTEX_LOCK:
      simgrid::xbt::intrusive_erase(simcall_mutex_lock__get__mutex(simcall)->sleeping, *process);
      break;

    case SIMCALL_COND_WAIT:
      simgrid::xbt::intrusive_erase(simcall_cond_wait__get__cond(simcall)->sleeping, *process);
      break;

    case SIMCALL_COND_WAIT_TIMEOUT:
      simgrid::xbt::intrusive_erase(simcall_cond_wait_timeout__get__cond(simcall)->sleeping, *process);
      simcall_cond_wait_timeout__set__result(simcall, 1); // signal a timeout
      break;

    case SIMCALL_SEM_ACQUIRE:
      simgrid::xbt::intrusive_erase(simcall_sem_acquire__get__sem(simcall)->sleeping_, *process);
      break;

    case SIMCALL_SEM_ACQUIRE_TIMEOUT:
      simgrid::xbt::intrusive_erase(simcall_sem_acquire_timeout__get__sem(simcall)->sleeping_, *process);
      simcall_sem_acquire_timeout__set__result(simcall, 1); // signal a timeout
      break;

    default:
      THROW_IMPOSSIBLE;
  }
  XBT_OUT();
}
