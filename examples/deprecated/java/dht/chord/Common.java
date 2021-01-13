/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package dht.chord;

public class Common {
  public static final int COMM_SIZE = 10;
  public static final int COMP_SIZE = 0;
  
  public static final int NB_BITS = 24;
  public static final int NB_KEYS = 16777216;
  public static final int TIMEOUT = 50;
  public static final int MAX_SIMULATION_TIME = 1000;
  public static final int PERIODIC_STABILIZE_DELAY = 20;
  public static final int PERIODIC_FIX_FINGERS_DELAY = 120;
  public static final int PERIODIC_CHECK_PREDECESSOR_DELAY = 120;
  public static final int PERIODIC_LOOKUP_DELAY = 10;
  private Common() {
    throw new IllegalAccessError("Utility class");
  }
}
