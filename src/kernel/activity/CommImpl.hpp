/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_COMM_HPP
#define SIMIX_SYNCHRO_COMM_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "surf/surf.hpp"

enum e_smx_comm_type_t { SIMIX_COMM_SEND, SIMIX_COMM_RECEIVE, SIMIX_COMM_READY, SIMIX_COMM_DONE };

namespace simgrid {
namespace kernel {
namespace activity {

XBT_PUBLIC_CLASS CommImpl : public ActivityImpl
{
  ~CommImpl() override;

public:
  explicit CommImpl(e_smx_comm_type_t type);
  void suspend() override;
  void resume() override;
  void post() override;
  void cancel();
  double remains();
  void cleanupSurf(); // FIXME: make me protected

  e_smx_comm_type_t type;       /* Type of the communication (SIMIX_COMM_SEND or SIMIX_COMM_RECEIVE) */
  smx_mailbox_t mbox = nullptr; /* Rendez-vous where the comm is queued */

#if SIMGRID_HAVE_MC
  smx_mailbox_t mbox_cpy = nullptr; /* Copy of the rendez-vous where the comm is queued, MC needs it for DPOR
                                     (comm.mbox set to nullptr when the communication is removed from the mailbox
                                     (used as garbage collector)) */
#endif
  bool detached = false; /* If detached or not */

  void (*clean_fun)(void*) = nullptr; /* Function to clean the detached src_buf if something goes wrong */
  int (*match_fun)(void*, void*, simgrid::kernel::activity::CommImpl*) =
      nullptr; /* Filter function used by the other side. It is used when
looking if a given communication matches my needs. For that, myself must match the
expectations of the other side, too. See  */
  void (*copy_data_fun)(smx_activity_t, void*, size_t) = nullptr;

  /* Surf action data */
  surf_action_t surfAction_ = nullptr; /* The Surf communication action encapsulated */
  surf_action_t src_timeout = nullptr; /* Surf's actions to instrument the timeouts */
  surf_action_t dst_timeout = nullptr; /* Surf's actions to instrument the timeouts */
  smx_actor_t src_proc      = nullptr;
  smx_actor_t dst_proc      = nullptr;
  double rate               = 0.0;
  double task_size          = 0.0;

  /* Data to be transfered */
  void* src_buff        = nullptr;
  void* dst_buff        = nullptr;
  size_t src_buff_size  = 0;
  size_t* dst_buff_size = nullptr;
  bool copied           = false; /* whether the data were already copied */

  void* src_data = nullptr; /* User data associated to communication */
  void* dst_data = nullptr;
};
}
}
} // namespace simgrid::kernel::activity

#endif
