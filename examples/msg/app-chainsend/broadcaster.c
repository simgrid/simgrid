/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "broadcaster.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_broadcaster, "Messages specific for the broadcaster");

xbt_dynar_t build_hostlist_from_hostcount(int hostcount)
{
  xbt_dynar_t host_list = xbt_dynar_new(sizeof(char*), xbt_free_ref);
  int i;
  
  for (i = 1; i <= hostcount; i++) {
    char *hostname = bprintf("host%d", i);
    XBT_DEBUG("%s", hostname);
    xbt_dynar_push(host_list, &hostname);
  }
  return host_list;
}

int broadcaster_build_chain(broadcaster_t bc)
{
  msg_task_t task = NULL;
  char **cur = (char**)xbt_dynar_iterator_next(bc->it);
  const char *me = "host0"; /* FIXME: hardcoded*/ /*MSG_host_get_name(MSG_host_self());*/
  const char *current_host = NULL;
  const char *prev = NULL;
  const char *next = NULL;
  const char *last = NULL;

  /* Build the chain if there's at least one peer */
  if (cur != NULL) {
    /* init: prev=NULL, host=current cur, next=next cur */
    next = *cur;
    bc->first = next;

    /* This iterator iterates one step ahead: cur is current iterated element, but is actually next in the chain */
    do {
      /* following steps: prev=last, host=next, next=cur */
      cur = (char**)xbt_dynar_iterator_next(bc->it);
      prev = last;
      current_host = next;
      if (cur != NULL)
        next = *cur;
      else
        next = NULL;
      XBT_DEBUG("Building chain--broadcaster:\"%s\" dest:\"%s\" prev:\"%s\" next:\"%s\"", me, current_host, prev, next);

      /* Send message to current peer */
      task = task_message_chain_new(prev, next, bc->piece_count);
      MSG_task_send(task, current_host);

      last = current_host;
    } while (cur != NULL);
  }

  return MSG_OK;
}

int broadcaster_send_file(broadcaster_t bc)
{
  const char *me = "host0"; /* FIXME: hardcoded*/ /*MSG_host_get_name(MSG_host_self());*/
  //msg_comm_t comm = NULL;
  msg_task_t task = NULL;

  bc->current_piece = 0;

  while (bc->current_piece < bc->piece_count) {
    task = task_message_data_new(NULL, PIECE_SIZE);
    XBT_DEBUG("Sending (send) piece %d from %s into mailbox %s", bc->current_piece, me, bc->first);
    MSG_task_send(task, bc->first);
    bc->current_piece++;
  }

  return MSG_OK;
}

broadcaster_t broadcaster_init(xbt_dynar_t host_list, unsigned int piece_count)
{
  int status;
  broadcaster_t bc = xbt_new(s_broadcaster_t, 1);

  bc->piece_count = piece_count;
  bc->current_piece = 0;
  bc->host_list = host_list;
  bc->it = xbt_dynar_iterator_new(bc->host_list, forward_indices_list);
  bc->max_pending_sends = MAX_PENDING_SENDS;
  bc->pending_sends = xbt_dynar_new(sizeof(msg_comm_t), NULL);

  status = broadcaster_build_chain(bc);
  xbt_assert(status == MSG_OK, "Chain initialization failed");

  return bc;
}

static void broadcaster_destroy(broadcaster_t bc)
{
  /* Destroy iterator and hostlist */
  xbt_dynar_iterator_delete(bc->it);
  xbt_dynar_free(&bc->pending_sends);
  xbt_dynar_free(&bc->host_list); /* FIXME: host names are not free'd */
  xbt_free(bc);
}

/** Emitter function  */
int broadcaster(int argc, char *argv[])
{
  broadcaster_t bc = NULL;
  xbt_dynar_t host_list = NULL;
  int status;
  unsigned int piece_count = PIECE_COUNT;

  XBT_DEBUG("broadcaster");

  /* Add every mailbox given by the hostcount in argv[1] to a dynamic array */
  host_list = build_hostlist_from_hostcount(xbt_str_parse_int(argv[1], "Invalid number of peers: %s"));

  /* argv[2] is the number of pieces */
  if (argc > 2) {
    piece_count = xbt_str_parse_int(argv[2], "Invalid number of pieces: %s");
    XBT_DEBUG("piece_count set to %d", piece_count);
  } else {
    XBT_DEBUG("No piece_count specified, defaulting to %d", piece_count);
  }
  bc = broadcaster_init(host_list, piece_count);

  /* TODO: Error checking */
  status = broadcaster_send_file(bc);

  broadcaster_destroy(bc);

  return status;
}
