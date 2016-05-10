/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_NETWORK_PRIVATE_H
#define _SIMIX_NETWORK_PRIVATE_H

#include <deque>
#include <xbt/base.h>

#include "simgrid/simix.h"
#include "popping_private.h"

/** @brief Rendez-vous point datatype */
typedef struct s_smx_mailbox {
  char *name;
  std::deque<smx_synchro_t> *comm_queue;
  smx_process_t permanent_receiver; //process which the mailbox is attached to
  std::deque<smx_synchro_t> *done_comm_queue;//messages already received in the permanent receive mode
} s_smx_mailbox_t;

XBT_PRIVATE void SIMIX_mailbox_exit(void);

XBT_PRIVATE smx_mailbox_t SIMIX_mbox_create(const char *name);
XBT_PRIVATE smx_mailbox_t SIMIX_mbox_get_by_name(const char *name);
XBT_PRIVATE void SIMIX_mbox_remove(smx_mailbox_t mbox, smx_synchro_t comm);

XBT_PRIVATE void SIMIX_mbox_set_receiver(smx_mailbox_t mbox, smx_process_t proc);
XBT_PRIVATE smx_synchro_t SIMIX_comm_irecv(smx_process_t dst_proc, smx_mailbox_t mbox,
                              void *dst_buff, size_t *dst_buff_size,
                              int (*)(void *, void *, smx_synchro_t),
                              void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                              void *data, double rate);
XBT_PRIVATE smx_synchro_t SIMIX_comm_iprobe(smx_process_t dst_proc, smx_mailbox_t mbox, int type, int src,
                              int tag, int (*match_fun)(void *, void *, smx_synchro_t), void *data);

#endif

