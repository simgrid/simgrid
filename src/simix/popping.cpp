/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/fifo.h"
#include "xbt/xbt_os_thread.h"
#if HAVE_MC
#include "src/mc/mc_private.h"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_popping, simix,
                                "Popping part of SIMIX (transmuting from user request into kernel handlers)");

void SIMIX_simcall_answer(smx_simcall_t simcall)
{
  if (simcall->issuer != simix_global->maestro_process){
    XBT_DEBUG("Answer simcall %s (%d) issued by %s (%p)", SIMIX_simcall_name(simcall->call), (int)simcall->call,
        simcall->issuer->name, simcall->issuer);
    simcall->issuer->simcall.call = SIMCALL_NONE;
/*    This check should be useless and slows everyone. Reactivate if you see something
 *    weird in process scheduling.
 */
/*    if(!xbt_dynar_member(simix_global->process_to_run, &(simcall->issuer))) */
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, simcall->issuer);
/*    else DIE_IMPOSSIBLE; */
  }
}

void SIMIX_simcall_exit(smx_synchro_t synchro)
{
  switch (synchro->type) {

    case SIMIX_SYNC_EXECUTE:
    case SIMIX_SYNC_PARALLEL_EXECUTE:
      SIMIX_post_host_execute(synchro);
      break;

    case SIMIX_SYNC_COMMUNICATE:
      SIMIX_post_comm(synchro);
      break;

    case SIMIX_SYNC_SLEEP:
      SIMIX_post_process_sleep(synchro);
      break;

    case SIMIX_SYNC_JOIN:
      SIMIX_post_process_sleep(synchro);
      break;

    case SIMIX_SYNC_SYNCHRO:
      SIMIX_post_synchro(synchro);
      break;

    case SIMIX_SYNC_IO:
      SIMIX_post_io(synchro);
      break;
  }
}

void SIMIX_run_kernel(void* code)
{
  std::function<void()>* function = (std::function<void()>*) code;
  (*function)();
}
