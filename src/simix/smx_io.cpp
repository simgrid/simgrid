/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

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

simgrid::kernel::activity::IoImplPtr SIMIX_io_start(std::string name, sg_size_t size, sg_storage_t storage,
                                                    simgrid::s4u::Io::OpType type)
{
  /* set surf's action */
  simgrid::kernel::resource::Action* surf_action = storage->get_impl()->io_start(size, type);

  simgrid::kernel::activity::IoImplPtr io =
      simgrid::kernel::activity::IoImplPtr(new simgrid::kernel::activity::IoImpl(name, surf_action, storage));

  XBT_DEBUG("Create IO synchro %p %s", io.get(), name.c_str());
  simgrid::kernel::activity::IoImpl::on_creation(io);

  return io;
}

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
        SMX_EXCEPTION(simcall->issuer, io_error, 0, "IO failed");
        break;
      case SIMIX_CANCELED:
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
        break;
      default:
        xbt_die("Internal error in SIMIX_io_finish: unexpected synchro state %d", static_cast<int>(synchro->state_));
    }

    if (simcall->issuer->host_->is_off()) {
      simcall->issuer->context_->iwannadie = 1;
    }

    simcall->issuer->waiting_synchro = nullptr;
    SIMIX_simcall_answer(simcall);
  }
}
