/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public abstract class CallbackActorHost {
  /** This method shall contain the code of your callback */
  public abstract void run(Actor e, Host h);

  /* Internal method to ensure that your callback can be called from C++ */
  public final void run(long a, long h) { run(new Actor(a, false), new Host(h)); }
}
