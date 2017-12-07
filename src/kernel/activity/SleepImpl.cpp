/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"

#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/context/Context.hpp"

#include "src/simix/ActorImpl.hpp"
#include "src/simix/popping_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_process);

void simgrid::kernel::activity::SleepImpl::suspend()
{
  surf_sleep->suspend();
}

void simgrid::kernel::activity::SleepImpl::resume()
{
  surf_sleep->resume();
}

void simgrid::kernel::activity::SleepImpl::post()
{
  while (not simcalls.empty()) {
    smx_simcall_t simcall = simcalls.front();
    simcalls.pop_front();

    e_smx_state_t result;
    switch (surf_sleep->getState()) {
      case simgrid::surf::Action::State::failed:
        simcall->issuer->context->iwannadie = 1;
        result                              = SIMIX_SRC_HOST_FAILURE;
        break;

      case simgrid::surf::Action::State::done:
        result = SIMIX_DONE;
        break;

      default:
        THROW_IMPOSSIBLE;
        break;
    }
    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }
    simcall_process_sleep__set__result(simcall, result);
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
