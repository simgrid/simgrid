/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package trace.pingpong;
import org.simgrid.msg.Task;

public class PingPongTask extends Task {
  private double timeVal;

  public PingPongTask() {
    this.timeVal = 0;
  }

  public PingPongTask(String name, double computeDuration, double messageSize) {
    super(name,computeDuration,messageSize);
  }

  public void setTime(double timeVal){
    this.timeVal = timeVal;
  }

  public double getTime() {
    return this.timeVal;
  }
}

