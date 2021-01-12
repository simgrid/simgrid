/* Copyright (c) 2012-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package dht.kademlia;

public class Common {
  /* Common constants used all over the simulation */
  public static final int COMM_SIZE = 1;
  public static final int COMP_SIZE = 0;

  public static final int RANDOM_LOOKUP_INTERVAL = 100;

  public static final int ALPHA = 3;

  public static final int IDENTIFIER_SIZE = 32;
  /* Maximum size of the buckets */
  public static final int BUCKET_SIZE = 20;
  /* Maximum number of trials for the "JOIN" request */
  public static final int MAX_JOIN_TRIALS = 4;
  /* Timeout for a "FIND_NODE" request to a node */
  public static final int FIND_NODE_TIMEOUT = 10;
  /* Global timeout for a FIND_NODE request */
  public static final int FIND_NODE_GLOBAL_TIMEOUT = 50;

  public static final int MAX_STEPS = 10;
  public static final int JOIN_BUCKETS_QUERIES = 1;
  private Common() {
    throw new IllegalAccessError("Utility class");
  }
}
