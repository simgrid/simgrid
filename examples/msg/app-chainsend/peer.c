/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "peer.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_peer, "Messages specific for the peer");

void peer_init_chain(peer_t peer, message_t msg)
{
  peer->prev = msg->prev_hostname;
  peer->next = msg->next_hostname;
  peer->total_pieces = msg->num_pieces;
  peer->init = 1;
}

static void peer_forward_msg(peer_t peer, message_t msg)
{
  msg_task_t task = task_message_data_new(NULL, msg->data_length);
  msg_comm_t comm = NULL;
  XBT_DEBUG("Sending (isend) from %s into mailbox %s", peer->me, peer->next);
  comm = MSG_task_isend(task, peer->next);
  queue_pending_connection(comm, peer->pending_sends);
}

int peer_execute_task(peer_t peer, msg_task_t task)
{
  int done = 0;
  message_t msg = MSG_task_get_data(task);
  
  XBT_DEBUG("Peer %s got message of type %d\n", peer->me, msg->type);
  switch (msg->type) {
    case MESSAGE_BUILD_CHAIN:
      peer_init_chain(peer, msg);
      break;
    case MESSAGE_SEND_DATA:
      xbt_assert(peer->init, "peer_execute_task() failed: got msg_type %d before initialization", msg->type);
      if (peer->next != NULL)
        peer_forward_msg(peer, msg);
      peer->pieces++;
      peer->bytes += msg->data_length;
      if (peer->pieces >= peer->total_pieces) {
        XBT_DEBUG("%d pieces receieved", peer->pieces);
        done = 1;
      }
      break;
  }

  MSG_task_execute(task);

  return done;
}

msg_error_t peer_wait_for_message(peer_t peer)
{
  msg_error_t status;
  msg_comm_t comm = NULL;
  msg_task_t task = NULL;
  int idx = -1;
  int done = 0;

  while (!done) {
    comm = MSG_task_irecv(&task, peer->me);
    queue_pending_connection(comm, peer->pending_recvs);

    if ((idx = MSG_comm_waitany(peer->pending_recvs)) != -1) {
      comm = xbt_dynar_get_as(peer->pending_recvs, idx, msg_comm_t);
      status = MSG_comm_get_status(comm);
      XBT_DEBUG("peer_wait_for_message: error code = %d", status);
      xbt_assert(status == MSG_OK, "peer_wait_for_message() failed");

      task = MSG_comm_get_task(comm);
      MSG_comm_destroy(comm);
      xbt_dynar_cursor_rm(peer->pending_recvs, (unsigned int*)&idx);
      done = peer_execute_task(peer, task);

      task_message_delete(task);
      task = NULL;
    }
    process_pending_connections(peer->pending_sends);
  }

  return status;
}

void peer_init(peer_t p, int argc, char *argv[])
{
  p->init = 0;
  p->prev = NULL;
  p->next = NULL;
  p->pieces = 0;
  p->bytes = 0;
  p->pending_recvs = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  p->pending_sends = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  /* Set mailbox name: use host number from argv or hostname if no argument given */
  if (argc > 1) {
    p->me = bprintf("host%s", argv[1]);
  } else {
    p->me = xbt_strdup(MSG_host_get_name(MSG_host_self()));
  }
}

void peer_shutdown(peer_t p)
{
  unsigned int size = xbt_dynar_length(p->pending_sends);
  unsigned int idx;
  msg_comm_t *comms = xbt_new(msg_comm_t, size);

  for (idx = 0; idx < size; idx++) {
    comms[idx] = xbt_dynar_get_as(p->pending_sends, idx, msg_comm_t);
  }

  XBT_DEBUG("Waiting for sends to finish before shutdown...");
  MSG_comm_waitall(comms, size, PEER_SHUTDOWN_DEADLINE);

  for (idx = 0; idx < size; idx++) {
    MSG_comm_destroy(comms[idx]);
  }

  xbt_free(comms);
}

void peer_delete(peer_t p)
{
  xbt_dynar_free(&p->pending_recvs);
  xbt_dynar_free(&p->pending_sends);
  xbt_free(p->me);
  xbt_free(p->prev);
  xbt_free(p->next);

  xbt_free(p);
}

void peer_print_stats(peer_t p, float elapsed_time)
{
  XBT_INFO("### %f %llu bytes (Avg %f MB/s); copy finished (simulated).", elapsed_time, p->bytes, p->bytes / 1024.0 / 1024.0 / elapsed_time); 
}

/** Peer function  */
int peer(int argc, char *argv[])
{
  float start_time, end_time;
  peer_t p = xbt_new(s_peer_t, 1);
  msg_error_t status;

  XBT_DEBUG("peer");

  peer_init(p, argc, argv);
  start_time = MSG_get_clock();
  status = peer_wait_for_message(p);
  peer_shutdown(p);
  end_time = MSG_get_clock();
  peer_print_stats(p, end_time - start_time);
  peer_delete(p);

  return status;
}
