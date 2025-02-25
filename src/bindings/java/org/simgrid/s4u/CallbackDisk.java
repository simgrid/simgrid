/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public abstract class CallbackDisk {
  /** This method shall contain the code of your callback */
  public abstract void run(Disk d);

  /* Internal method to ensure that your callback can be called from C++ */
  public final void run(long d) { run(new Disk(d)); }
}
