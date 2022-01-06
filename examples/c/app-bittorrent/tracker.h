/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_TRACKER_H
#define BITTORRENT_TRACKER_H
#include "app-bittorrent.h"
#include <xbt/dynar.h>

void tracker(int argc, char* argv[]);
/** Tasks exchanged between a tracker and peers. */
typedef struct s_tracker_query {
  int peer_id;                 // peer id
  sg_mailbox_t return_mailbox; // mailbox where the tracker should answer
} s_tracker_query_t;
typedef s_tracker_query_t* tracker_query_t;

typedef struct s_tracker_answer {
  int interval;      // how often the peer should contact the tracker (unused for now)
  xbt_dynar_t peers; // the peer list the peer has asked for.
} s_tracker_answer_t;
typedef s_tracker_answer_t* tracker_answer_t;
typedef const s_tracker_answer_t* const_tracker_answer_t;

tracker_query_t tracker_query_new(int peer_id, sg_mailbox_t return_mailbox);
tracker_answer_t tracker_answer_new(int interval);
void tracker_answer_free(void* data);

#endif /* BITTORRENT_TRACKER_H */
