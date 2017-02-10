/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_NETWORK_PRIVATE_H
#define _SIMIX_NETWORK_PRIVATE_H

#include <boost/circular_buffer.hpp>

#include "simgrid/s4u/Mailbox.hpp"

#include "src/simix/ActorImpl.hpp"


#define MAX_MAILBOX_SIZE 10000000
namespace simgrid {
namespace simix {

/** @brief Rendez-vous point datatype */

class MailboxImpl {
public:
  MailboxImpl(const char* name)
      : piface_(this), name(xbt_strdup(name)), comm_queue(MAX_MAILBOX_SIZE), done_comm_queue(MAX_MAILBOX_SIZE)
  {
  }
  ~MailboxImpl() { xbt_free(name); }

  simgrid::s4u::Mailbox piface_; // Our interface
  char* name;
  boost::circular_buffer_space_optimized<smx_activity_t> comm_queue;
  boost::intrusive_ptr<simgrid::simix::ActorImpl> permanent_receiver; //process which the mailbox is attached to
  boost::circular_buffer_space_optimized<smx_activity_t> done_comm_queue;//messages already received in the permanent receive mode
};

}
}

XBT_PRIVATE void SIMIX_mailbox_exit();

XBT_PRIVATE smx_mailbox_t SIMIX_mbox_create(const char *name);
XBT_PRIVATE smx_mailbox_t SIMIX_mbox_get_by_name(const char *name);
XBT_PRIVATE void SIMIX_mbox_remove(smx_mailbox_t mbox, smx_activity_t comm);

XBT_PRIVATE void SIMIX_mbox_set_receiver(smx_mailbox_t mbox, smx_actor_t proc);
XBT_PRIVATE smx_activity_t SIMIX_comm_irecv(smx_actor_t dst_proc, smx_mailbox_t mbox,
                              void *dst_buff, size_t *dst_buff_size,
                              int (*match_fun)(void *, void *, smx_activity_t),
                              void (*copy_data_fun)(smx_activity_t, void*, size_t),
                              void *data, double rate);
XBT_PRIVATE smx_activity_t SIMIX_comm_iprobe(smx_actor_t dst_proc, smx_mailbox_t mbox, int type, int src,
                              int tag, int (*match_fun)(void *, void *, smx_activity_t), void *data);

#endif
