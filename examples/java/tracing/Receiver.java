/* Copyright (c) 2006-2007, 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package tracing;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.trace.Trace;

public class Receiver extends Process {
  private  final double commSizeLat = 1;
  private final double commSizeBw = 100000000;

  public Receiver(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("hello!");
    Trace.hostPushState (getHost().getName(), "PM_STATE", "waitingPing");
    double communicationTime=0;

    double time = Msg.getClock();

    /* Wait for the ping */ 
    Msg.info("try to get a task");

    PingPongTask ping = (PingPongTask)Task.receive(getHost().getName());
    double timeGot = Msg.getClock();
    double timeSent = ping.getTime();

    Msg.info("Got at time "+ timeGot);
    Msg.info("Was sent at time "+timeSent);
    time=timeSent;

    communicationTime=timeGot - time;
    Msg.info("Communication time : " + communicationTime);

    Msg.info(" --- bw "+ commSizeBw/communicationTime + " ----");

    /* Send the pong */
    Trace.hostPushState (getHost().getName(), "PM_STATE", "sendingPong");
    double computeDuration = 0;
    PingPongTask pong = new PingPongTask("no name",computeDuration,commSizeLat);
    pong.setTime(time);
    pong.send(ping.getSource().getName());

    /* Pop the two states */
    Trace.hostPopState (getHost().getName(), "PM_STATE");
    Trace.hostPopState (getHost().getName(), "PM_STATE");

    Msg.info("goodbye!");
  }
}
