/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Io.hpp"

#include "smx_private.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_io_private.hpp"
#include "src/surf/StorageImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_io, simix, "Logging specific to SIMIX (io)");

void simcall_HANDLER_io_wait(smx_simcall_t simcall, smx_activity_t synchro)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro.get(), (int)synchro->state_);

  /* Associate this simcall to the synchro */
  synchro->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  /* set surf's synchro */
  if (MC_is_active() || MC_record_replay_is_active()) {
    synchro->state_ = SIMIX_DONE;
    SIMIX_io_finish(synchro);
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (synchro->state_ != SIMIX_RUNNING)
    SIMIX_io_finish(synchro);
}

void SIMIX_io_finish(smx_activity_t synchro)
{
  for (smx_simcall_t const& simcall : synchro->simcalls_) {
    switch (synchro->state_) {
      case SIMIX_DONE:
        /* do nothing, synchro done */
        break;
      case SIMIX_FAILED:
        simcall->issuer->exception_ =
            std::make_exception_ptr(simgrid::StorageFailureException(XBT_THROW_POINT, "Storage failed"));
        break;
      case SIMIX_CANCELED:
        simcall->issuer->exception_ =
            std::make_exception_ptr(simgrid::CancelException(XBT_THROW_POINT, "I/O Canceled"));
        break;
      default:
        xbt_die("Internal error in SIMIX_io_finish: unexpected synchro state %d", static_cast<int>(synchro->state_));
    }

    simcall->issuer->waiting_synchro = nullptr;
    if (simcall->issuer->get_host()->is_on())
      SIMIX_simcall_answer(simcall);
    else
      simcall->issuer->context_->iwannadie = true;
  }
}
