/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SynchroIo.hpp"
#include "simgrid/kernel/resource/Action.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/surf_interface.hpp"

void simgrid::kernel::activity::IoImpl::suspend()
{
  if (surf_io)
    surf_io->suspend();
}

void simgrid::kernel::activity::IoImpl::resume()
{
  if (surf_io)
    surf_io->resume();
}

void simgrid::kernel::activity::IoImpl::post()
{
  for (smx_simcall_t const& simcall : simcalls) {
    switch (simcall->call) {
      case SIMCALL_STORAGE_WRITE:
        simcall_storage_write__set__result(simcall, surf_io->getCost());
        break;
      case SIMCALL_STORAGE_READ:
        simcall_storage_read__set__result(simcall, surf_io->getCost());
        break;
      default:
        break;
    }
  }

  switch (surf_io->getState()) {
    case simgrid::kernel::resource::Action::State::failed:
      state = SIMIX_FAILED;
      break;
    case simgrid::kernel::resource::Action::State::done:
      state = SIMIX_DONE;
      break;
    default:
      THROW_IMPOSSIBLE;
      break;
  }

  SIMIX_io_finish(this);
}
