/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/log.h>

#include <simgrid/s4u/host.hpp>

#include "src/kernel/activity/SynchroSleep.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simix/popping_private.h"
#include "src/simix/smx_process_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

void simgrid::kernel::activity::Sleep::suspend()
{
  surf_sleep->suspend();
}

void simgrid::kernel::activity::Sleep::resume()
{
  surf_sleep->resume();
}

void simgrid::kernel::activity::Sleep::post()
{
  while (!simcalls.empty()) {
    smx_simcall_t simcall = simcalls.front();
    simcalls.pop_front();

    e_smx_state_t state;
    switch (surf_sleep->getState()){
      case simgrid::surf::Action::State::failed:
        simcall->issuer->context->iwannadie = 1;
        //SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
        state = SIMIX_SRC_HOST_FAILURE;
        break;

      case simgrid::surf::Action::State::done:
        state = SIMIX_DONE;
        break;

      default:
        THROW_IMPOSSIBLE;
        break;
    }
    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }
    simcall_process_sleep__set__result(simcall, state);
    simcall->issuer->waiting_synchro = nullptr;
    if (simcall->issuer->suspended) {
      XBT_DEBUG("Wait! This process is suspended and can't wake up now.");
      simcall->issuer->suspended = 0;
      simcall_HANDLER_process_suspend(simcall, simcall->issuer);
    } else {
      SIMIX_simcall_answer(simcall);
    }
  }

  SIMIX_process_sleep_destroy(this);
}
