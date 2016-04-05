/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef BITTORRENT_BITTORRENT_H_
#define BITTORRENT_BITTORRENT_H_

#define MAILBOX_SIZE 40
#define TRACKER_MAILBOX "tracker_mailbox"
/** Max number of pairs sent by the tracker to clients */
#define MAXIMUM_PAIRS 50
/** Interval of time where the peer should send a request to the tracker */
#define TRACKER_QUERY_INTERVAL 1000
/** Communication size for a task to the tracker */
#define TRACKER_COMM_SIZE 0.01
#define GET_PEERS_TIMEOUT 10000
#define TIMEOUT_MESSAGE 10
#define TRACKER_RECEIVE_TIMEOUT 10
/** Number of peers that can be unchocked at a given time */
#define MAX_UNCHOKED_PEERS 4
/** Interval between each update of the choked peers */
#define UPDATE_CHOKED_INTERVAL 30
/** Number of pieces the peer asks for simultaneously */
#define MAX_PIECES 1

#endif                          /* BITTORRENT_BITTORRENT_H_ */
