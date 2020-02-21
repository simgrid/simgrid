/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "chainsend.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(chainsend_peer, "Messages specific for the peer");

static void peer_join_chain(peer_t p)
{
  chain_message_t msg = (chain_message_t)sg_mailbox_get(p->me);
  p->prev             = msg->prev_;
  p->next             = msg->next_;
  p->total_pieces     = msg->num_pieces;
  XBT_DEBUG("Peer %s got a 'BUILD_CHAIN' message (prev: %s / next: %s)", sg_mailbox_get_name(p->me),
            p->prev ? sg_mailbox_get_name(p->prev) : NULL, p->next ? sg_mailbox_get_name(p->next) : NULL);
  free(msg);
}

static void peer_forward_file(peer_t p)
{
  void* received;
  int done                = 0;
  size_t nb_pending_sends = 0;
  size_t nb_pending_recvs = 0;

  while (!done) {
    p->pending_recvs[nb_pending_recvs] = sg_mailbox_get_async(p->me, &received);
    nb_pending_recvs++;

    int idx = sg_comm_wait_any(p->pending_recvs, nb_pending_recvs);
    if (idx != -1) {
      XBT_DEBUG("Peer %s got a 'SEND_DATA' message", sg_mailbox_get_name(p->me));
      /* move the last pending comm where the finished one was, and decrement */
      p->pending_recvs[idx] = p->pending_recvs[--nb_pending_recvs];

      if (p->next != NULL) {
        XBT_DEBUG("Sending %s (asynchronously) from %s to %s", (char*)received, sg_mailbox_get_name(p->me),
                  sg_mailbox_get_name(p->next));
        sg_comm_t send = sg_mailbox_put_async(p->next, received, MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
        p->pending_sends[nb_pending_sends] = send;
        nb_pending_sends++;
      } else
        free(received);

      p->received_pieces++;
      p->received_bytes += PIECE_SIZE;
      XBT_DEBUG("%u pieces received, %llu bytes received", p->received_pieces, p->received_bytes);
      if (p->received_pieces >= p->total_pieces) {
        done = 1;
      }
    }
  }
  sg_comm_wait_all(p->pending_sends, nb_pending_sends);
}

static peer_t peer_init(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  peer_t p           = (peer_t)malloc(sizeof(s_peer_t));
  p->prev            = NULL;
  p->next            = NULL;
  p->received_pieces = 0;
  p->received_bytes  = 0;
  p->pending_recvs   = (sg_comm_t*)malloc(sizeof(sg_comm_t) * MAX_PENDING_COMMS);
  p->pending_sends   = (sg_comm_t*)malloc(sizeof(sg_comm_t) * MAX_PENDING_COMMS);

  p->me = sg_mailbox_by_name(sg_host_self_get_name());

  return p;
}

static void peer_delete(peer_t p)
{
  free(p->pending_recvs);
  free(p->pending_sends);

  free(p);
}

void peer(int argc, char* argv[])
{
  XBT_DEBUG("peer");

  peer_t p          = peer_init(argc, argv);
  double start_time = simgrid_get_clock();
  peer_join_chain(p);
  peer_forward_file(p);
  double end_time = simgrid_get_clock();

  XBT_INFO("### %f %llu bytes (Avg %f MB/s); copy finished (simulated).", end_time - start_time, p->received_bytes,
           p->received_bytes / 1024.0 / 1024.0 / (end_time - start_time));

  peer_delete(p);
}
