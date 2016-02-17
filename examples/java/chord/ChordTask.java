/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package chord;

import chord.Common;
import org.simgrid.msg.Task;

public class ChordTask extends Task {
  public String issuerHostName;
  public String answerTo;
  public ChordTask() {
    this(null,null);
  }
  public ChordTask(String issuerHostName, String answerTo) {
    super(null, Common.COMP_SIZE, Common.COMM_SIZE);
    this.issuerHostName = issuerHostName;
    this.answerTo = answerTo;
  }
}
