/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_NETWORK_PRIVATE_H
#define _SIMIX_NETWORK_PRIVATE_H

#include <xbt/base.h>

#include "simgrid/simix.h"
#include "popping_private.h"

/** @brief Rendez-vous point datatype */
typedef struct s_smx_rvpoint {
  char *name;
  xbt_fifo_t comm_fifo;
  void *data;
  smx_process_t permanent_receiver; //process which the mailbox is attached to
  xbt_fifo_t done_comm_fifo;//messages already received in the permanent receive mode
} s_smx_rvpoint_t;

XBT_PRIVATE void SIMIX_network_init(void);
XBT_PRIVATE void SIMIX_network_exit(void);

XBT_PRIVATE smx_mailbox_t SIMIX_rdv_create(const char *name);
XBT_PRIVATE void SIMIX_rdv_destroy(smx_mailbox_t rdv);
XBT_PRIVATE smx_mailbox_t SIMIX_rdv_get_by_name(const char *name);
XBT_PRIVATE void SIMIX_rdv_remove(smx_mailbox_t rdv, smx_synchro_t comm);
XBT_PRIVATE int SIMIX_rdv_comm_count_by_host(smx_mailbox_t rdv, sg_host_t host);
XBT_PRIVATE smx_synchro_t SIMIX_rdv_get_head(smx_mailbox_t rdv);
XBT_PRIVATE void SIMIX_rdv_set_receiver(smx_mailbox_t rdv, smx_process_t proc);
XBT_PRIVATE smx_process_t SIMIX_rdv_get_receiver(smx_mailbox_t rdv);
XBT_PRIVATE smx_synchro_t SIMIX_comm_irecv(smx_process_t dst_proc, smx_mailbox_t rdv,
                              void *dst_buff, size_t *dst_buff_size,
                              int (*)(void *, void *, smx_synchro_t),
                              void (*copy_data_fun)(smx_synchro_t, void*, size_t),
                              void *data, double rate);
XBT_PRIVATE void SIMIX_comm_destroy(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_comm_destroy_internal_actions(smx_synchro_t synchro);
XBT_PRIVATE smx_synchro_t SIMIX_comm_iprobe(smx_process_t dst_proc, smx_mailbox_t rdv, int type, int src,
                              int tag, int (*match_fun)(void *, void *, smx_synchro_t), void *data);
XBT_PRIVATE void SIMIX_post_comm(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_comm_cancel(smx_synchro_t synchro);
XBT_PRIVATE double SIMIX_comm_get_remains(smx_synchro_t synchro);
XBT_PRIVATE e_smx_state_t SIMIX_comm_get_state(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_comm_suspend(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_comm_resume(smx_synchro_t synchro);
XBT_PRIVATE smx_process_t SIMIX_comm_get_src_proc(smx_synchro_t synchro);
XBT_PRIVATE smx_process_t SIMIX_comm_get_dst_proc(smx_synchro_t synchro);

#endif

