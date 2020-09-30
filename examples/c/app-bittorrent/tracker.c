/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "tracker.h"

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(bittorrent_tracker, "Messages specific for the tracker");

void tracker_answer_free(void* data)
{
  tracker_answer_t a = (tracker_answer_t)data;
  xbt_dynar_free(&a->peers);
  free(a);
}

static int is_in_list(const_xbt_dynar_t peers, int id)
{
  return xbt_dynar_member(peers, &id);
}

void tracker(int argc, char* argv[])
{
  // Checking arguments
  xbt_assert(argc == 2, "Wrong number of arguments for the tracker.");
  // Retrieving end time
  double deadline = xbt_str_parse_double(argv[1], "Invalid deadline: %s");
  xbt_assert(deadline > 0, "Wrong deadline supplied");

  // Building peers array
  xbt_dynar_t peers_list = xbt_dynar_new(sizeof(int), NULL);

  sg_mailbox_t mailbox = sg_mailbox_by_name(TRACKER_MAILBOX);

  XBT_INFO("Tracker launched.");
  sg_comm_t comm_received = NULL;
  void* received          = NULL;

  while (simgrid_get_clock() < deadline) {
    if (comm_received == NULL)
      comm_received = sg_mailbox_get_async(mailbox, &received);

    if (sg_comm_test(comm_received)) {
      // Retrieve the data sent by the peer.
      xbt_assert(received != NULL);
      tracker_query_t tq = (tracker_query_t)received;

      // Add the peer to our peer list.
      if (!is_in_list(peers_list, tq->peer_id))
        xbt_dynar_push_as(peers_list, int, tq->peer_id);

      // Sending peers to the requesting peer
      tracker_answer_t ta = tracker_answer_new(TRACKER_QUERY_INTERVAL);
      int next_peer;
      int peers_length = (int)xbt_dynar_length(peers_list);
      for (int i = 0; i < MAXIMUM_PEERS && i < peers_length; i++) {
        do {
          next_peer = xbt_dynar_get_as(peers_list, rand() % peers_length, int);
        } while (is_in_list(ta->peers, next_peer));
        xbt_dynar_push_as(ta->peers, int, next_peer);
      }
      // sending the task back to the peer.
      sg_comm_t answer = sg_mailbox_put_init(tq->return_mailbox, ta, TRACKER_COMM_SIZE);
      sg_comm_detach(answer, tracker_answer_free);

      xbt_free(tq);
      comm_received = NULL;
      received      = NULL;
    } else {
      sg_actor_sleep_for(1);
    }
  }
  // Free the remaining communication if any
  if (comm_received)
    sg_comm_unref(comm_received);
  // Free the peers list
  xbt_dynar_free(&peers_list);

  XBT_INFO("Tracker is leaving");
}

tracker_query_t tracker_query_new(int peer_id, sg_mailbox_t return_mailbox)
{
  tracker_query_t tq = xbt_new(s_tracker_query_t, 1);
  tq->peer_id        = peer_id;
  tq->return_mailbox = return_mailbox;
  return tq;
}

tracker_answer_t tracker_answer_new(int interval)
{
  tracker_answer_t ta = xbt_new(s_tracker_answer_t, 1);
  ta->interval        = interval;
  ta->peers           = xbt_dynar_new(sizeof(int), NULL);

  return ta;
}
