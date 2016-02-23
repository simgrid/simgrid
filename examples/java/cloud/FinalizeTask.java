/* Copyright (c) 2012-2014,2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud;

import org.simgrid.msg.Task;

public class FinalizeTask extends Task {
  public FinalizeTask(double compSize, double commSize) {
    super("Finalize",compSize,commSize);
  }
}