/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "chainsend.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(broadcaster, "Messages specific for the broadcaster");

static chain_message_t chain_message_new(sg_mailbox_t prev, sg_mailbox_t next, const unsigned int num_pieces)
{
  chain_message_t msg = (chain_message_t)malloc(sizeof(s_chain_message_t));
  msg->prev_          = prev;
  msg->next_          = next;
  msg->num_pieces     = num_pieces;

  return msg;
}

static void broadcaster_build_chain(broadcaster_t bc)
{
  /* Build the chain if there's at least one peer */
  if (bc->host_count > 0)
    bc->first = bc->mailboxes[0];

  for (unsigned i = 0; i < bc->host_count; i++) {
    sg_mailbox_t prev = i > 0 ? bc->mailboxes[i - 1] : NULL;
    sg_mailbox_t next = i < bc->host_count - 1 ? bc->mailboxes[i + 1] : NULL;
    XBT_DEBUG("Building chain--broadcaster:\"%s\" dest:\"%s\" prev:\"%s\" next:\"%s\"", sg_host_self_get_name(),
              sg_mailbox_get_name(bc->mailboxes[i]), prev ? sg_mailbox_get_name(prev) : NULL,
              next ? sg_mailbox_get_name(next) : NULL);
    /* Send message to current peer */
    sg_mailbox_put(bc->mailboxes[i], chain_message_new(prev, next, bc->piece_count), MESSAGE_BUILD_CHAIN_SIZE);
  }
}

static void broadcaster_send_file(const_broadcaster_t bc)
{
  int nb_pending_sends = 0;

  for (unsigned int current_piece = 0; current_piece < bc->piece_count; current_piece++) {
    XBT_DEBUG("Sending (send) piece %u from %s into mailbox %s", current_piece, sg_host_self_get_name(),
              sg_mailbox_get_name(bc->first));
    char* file_piece = bprintf("piece-%u", current_piece);
    sg_comm_t comm   = sg_mailbox_put_async(bc->first, file_piece, MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
    bc->pending_sends[nb_pending_sends++] = comm;
  }
  sg_comm_wait_all(bc->pending_sends, nb_pending_sends);
}

static broadcaster_t broadcaster_init(sg_mailbox_t* mailboxes, unsigned int host_count, unsigned int piece_count)
{
  broadcaster_t bc = (broadcaster_t)malloc(sizeof(s_broadcaster_t));

  bc->first         = NULL;
  bc->host_count    = host_count;
  bc->piece_count   = piece_count;
  bc->mailboxes     = mailboxes;
  bc->pending_sends = (sg_comm_t*)malloc(sizeof(sg_comm_t) * MAX_PENDING_COMMS);

  broadcaster_build_chain(bc);

  return bc;
}

static void broadcaster_destroy(broadcaster_t bc)
{
  free(bc->pending_sends);
  free(bc->mailboxes);
  free(bc);
}

/** Emitter function  */
void broadcaster(int argc, char* argv[])
{
  XBT_DEBUG("broadcaster");
  xbt_assert(argc > 2);
  unsigned int host_count = xbt_str_parse_int(argv[1], "Invalid number of peers: %s");

  sg_mailbox_t* mailboxes = (sg_mailbox_t*)malloc(sizeof(sg_mailbox_t) * host_count);

  for (unsigned int i = 1; i <= host_count; i++) {
    char* name = bprintf("node-%u.simgrid.org", i);
    XBT_DEBUG("%s", name);
    mailboxes[i - 1] = sg_mailbox_by_name(name);
    free(name);
  }

  unsigned int piece_count = xbt_str_parse_int(argv[2], "Invalid number of pieces: %s");

  broadcaster_t bc = broadcaster_init(mailboxes, host_count, piece_count);

  broadcaster_send_file(bc);

  broadcaster_destroy(bc);
}
