/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_TRACKER_H_
#define BITTORRENT_TRACKER_H_
#include "bittorrent.h"
#include <xbt/dynar.h>
/**
 * Tracker main function
 */
int tracker(int argc, char* argv[]);
/**
 * Task types exchanged between a node and the tracker
 */
typedef enum { TRACKER_TASK_QUERY, TRACKER_TASK_ANSWER } e_tracker_task_type_t;
/**
 * Tasks exchanged between a tracker and peers.
 */
typedef struct s_tracker_task_data {
  e_tracker_task_type_t type;   // type of the task
  const char* mailbox;          // mailbox where the tracker should answer
  const char* issuer_host_name; // hostname, for debug purposes
  // Query data
  int peer_id;    // peer id
  int uploaded;   // how much the peer has already uploaded
  int downloaded; // how much the peer has downloaded
  int left;       // how much the peer has left
  // Answer data
  int interval;      // how often the peer should contact the tracker (unused for now)
  xbt_dynar_t peers; // the peer list the peer has asked for.
} s_tracker_task_data_t;
typedef s_tracker_task_data_t* tracker_task_data_t;

tracker_task_data_t tracker_task_data_new(const char* issuer_host_name, const char* mailbox, int peer_id, int uploaded,
                                          int downloaded, int left);
void tracker_task_data_free(tracker_task_data_t task);

int is_in_list(const_xbt_dynar_t peers, int id);
#endif /* BITTORRENT_TRACKER_H */
