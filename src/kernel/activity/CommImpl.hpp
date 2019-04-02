/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_COMM_HPP
#define SIMGRID_KERNEL_ACTIVITY_COMM_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "surf/surf.hpp"


namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC CommImpl : public ActivityImpl_T<CommImpl> {
  ~CommImpl() override;
  void cleanupSurf();

  double rate_ = 0.0;
  double size_ = 0.0;

public:
  enum class Type { SEND = 0, RECEIVE, READY, DONE };

  CommImpl& set_type(CommImpl::Type type);
  CommImpl& set_size(double size);
  double get_rate() { return rate_; }
  CommImpl& set_rate(double rate);
  CommImpl& set_src_buff(void* buff, size_t size);
  CommImpl& set_dst_buff(void* buff, size_t* size);

  CommImpl* start();
  void copy_data();
  void suspend() override;
  void resume() override;
  void post() override;
  void finish() override;
  void cancel();
  double remains();

  CommImpl::Type type_;        /* Type of the communication (SIMIX_COMM_SEND or SIMIX_COMM_RECEIVE) */
  MailboxImpl* mbox = nullptr; /* Rendez-vous where the comm is queued */

#if SIMGRID_HAVE_MC
  MailboxImpl* mbox_cpy = nullptr; /* Copy of the rendez-vous where the comm is queued, MC needs it for DPOR
                                     (comm.mbox set to nullptr when the communication is removed from the mailbox
                                     (used as garbage collector)) */
#endif
  bool detached = false; /* If detached or not */

  void (*clean_fun)(void*) = nullptr; /* Function to clean the detached src_buf if something goes wrong */
  int (*match_fun)(void*, void*, CommImpl*) = nullptr; /* Filter function used by the other side. It is used when
looking if a given communication matches my needs. For that, myself must match the
expectations of the other side, too. See  */
  void (*copy_data_fun)(CommImpl*, void*, size_t) = nullptr;

  /* Surf action data */
  resource::Action* src_timeout_ = nullptr; /* Surf's actions to instrument the timeouts */
  resource::Action* dst_timeout_ = nullptr; /* Surf's actions to instrument the timeouts */
  actor::ActorImplPtr src_actor_ = nullptr;
  actor::ActorImplPtr dst_actor_ = nullptr;

  /* Data to be transfered */
  void* src_buff_        = nullptr;
  void* dst_buff_        = nullptr;
  size_t src_buff_size_  = 0;
  size_t* dst_buff_size_ = nullptr;
  bool copied           = false; /* whether the data were already copied */

  void* src_data_ = nullptr; /* User data associated to the communication */
  void* dst_data_ = nullptr;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
