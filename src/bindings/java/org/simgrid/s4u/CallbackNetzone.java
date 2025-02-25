/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public abstract class CallbackNetzone {
  /** This method shall contain the code of your callback */
  public abstract void run(NetZone n);

  /* Internal method to ensure that your callback can be called from C++ */
  public final void run(long e) { run(new NetZone(e)); }
}