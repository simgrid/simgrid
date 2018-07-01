/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_popping, simix,
                                "Popping part of SIMIX (transmuting from user request into kernel handlers)");

void SIMIX_simcall_answer(smx_simcall_t simcall)
{
  if (simcall->issuer != simix_global->maestro_process){
    XBT_DEBUG("Answer simcall %s (%d) issued by %s (%p)", SIMIX_simcall_name(simcall->call), (int)simcall->call,
              simcall->issuer->get_cname(), simcall->issuer);
    simcall->issuer->simcall.call = SIMCALL_NONE;
#if 0
    /* This check should be useless and slows everyone. Reactivate if you see something weird in process scheduling. */
    if (std::find(begin(simix_global->process_to_run), end(simix_global->process_to_run), simcall->issuer) !=
        end(simix_global->process_to_run))
      DIE_IMPOSSIBLE;
#endif
    simix_global->process_to_run.push_back(simcall->issuer);
  }
}

void SIMIX_simcall_exit(smx_activity_t activity)
{
  if (activity != nullptr) // When migrating, the surf activity is disconnected from its simix activity before cancel
    activity->post();
}

void SIMIX_run_kernel(std::function<void()> const* code)
{
  (*code)();
}

/** Kernel code for run_blocking
 *
 * The implementtion looks a lot like SIMIX_run_kernel ^^
 *
 * However, this `run_blocking` is blocking so the process will not be woken
 * up until `SIMIX_simcall_answer(simcall)`` is called by the kernel.
 * This means that `code` is responsible for doing this.
 */
void SIMIX_run_blocking(std::function<void()> const* code)
{
  (*code)();
}
