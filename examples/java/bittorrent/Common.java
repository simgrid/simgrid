/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package bittorrent;

/* Common constants for use in the simulation */
public class Common {
  public static final String TRACKER_MAILBOX = "tracker_mailbox";
  public static final int FILE_SIZE = 5120;
  public static final int FILE_PIECE_SIZE = 512;
  public static final int FILE_PIECES = 10;
  public static final int PIECES_BLOCKS = 5;
  public static final int BLOCKS_REQUESTED = 2;
  public static final int PIECE_COMM_SIZE = 1;
  public static final int MESSAGE_SIZE = 1; /* Information message size */
  public static final int MAXIMUM_PEERS = 50;  /* Max number of peers sent by the tracker to clients */
  public static final int TRACKER_QUERY_INTERVAL = 1000;  /* Interval of time where the peer should send a request to the tracker */
  public static final double TRACKER_COMM_SIZE = 0.01;  /* Communication size for a task to the tracker */
  public static final int GET_PEERS_TIMEOUT = 10000;  /* Timeout for the get peers data */
  public static final int TIMEOUT_MESSAGE = 10;
  public static final int TRACKER_RECEIVE_TIMEOUT = 10;
  public static final int MAX_UNCHOKED_PEERS = 4;  /* Number of peers that can be unchocked at a given time */
  public static final int UPDATE_CHOKED_INTERVAL = 30;  /* Interval between each update of the choked peers */
  public static final int MAX_PIECES = 1;  /* Number of pieces the peer asks for simultaneously */
}
