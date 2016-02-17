/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;

public class Common {
  /* Common constants used all over the simulation */
  public final static int COMM_SIZE = 1;
  public final static int COMP_SIZE = 0;

  public final static int RANDOM_LOOKUP_INTERVAL = 100;

  public final static int alpha = 3;

  public final static int IDENTIFIER_SIZE = 32;
  /* Maximum size of the buckets */
  public final static int BUCKET_SIZE = 20;
  /* Maximum number of trials for the "JOIN" request */
  public final static int MAX_JOIN_TRIALS = 4;
  /* Timeout for a "FIND_NODE" request to a node */
  public final static int FIND_NODE_TIMEOUT = 10;
  /* Global timeout for a FIND_NODE request */
  public final static int FIND_NODE_GLOBAL_TIMEOUT = 50;
  /* Timeout for a "PING" request */
  public final static int PING_TIMEOUT = 35;

  public final static int MAX_STEPS = 10;
  public final static int JOIN_BUCKETS_QUERIES = 1;
}
