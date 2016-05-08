/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/simix/SynchroComm.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/simix/smx_network_private.h"
#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_network);

simgrid::simix::Comm::Comm(e_smx_comm_type_t _type) {
  state = SIMIX_WAITING;
  this->type = _type;
  refcount = 1;
  src_data=NULL;
  dst_data=NULL;

  XBT_DEBUG("Create communicate synchro %p", this);
}
void simgrid::simix::Comm::suspend() {
  /* FIXME: shall we suspend also the timeout synchro? */
  if (surf_comm)
    surf_comm->suspend();
  /* in the other case, the action will be suspended on creation, in SIMIX_comm_start() */

}

void simgrid::simix::Comm::resume() {
  /*FIXME: check what happen with the timeouts */
  if (surf_comm)
    surf_comm->resume();
  /* in the other case, the synchro were not really suspended yet, see SIMIX_comm_suspend() and SIMIX_comm_start() */
}

void simgrid::simix::Comm::cancel() {
  /* if the synchro is a waiting state means that it is still in a mbox */
  /* so remove from it and delete it */
  if (state == SIMIX_WAITING) {
    SIMIX_mbox_remove(mbox, this);
    state = SIMIX_CANCELED;
  }
  else if (!MC_is_active() /* when running the MC there are no surf actions */
           && !MC_record_replay_is_active()
           && (state == SIMIX_READY || state == SIMIX_RUNNING)) {

    surf_comm->cancel();
  }
}
