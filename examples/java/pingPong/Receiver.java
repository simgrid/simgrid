/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package pingPong;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Receiver extends Process {
  final double commSizeLat = 1;
  final double commSizeBw = 100000000;
  public Receiver(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("hello!");
    double communicationTime=0;

    double time = Msg.getClock();

    Msg.info("try to get a task");

    PingPongTask task = (PingPongTask)Task.receive(getHost().getName());
    double timeGot = Msg.getClock();
    double timeSent = task.getTime();

    Msg.info("Got at time "+ timeGot);
    Msg.info("Was sent at time "+timeSent);
    time=timeSent;

    communicationTime=timeGot - time;
    Msg.info("Communication time : " + communicationTime);
    Msg.info(" --- bw "+ commSizeBw/communicationTime + " ----");
    Msg.info("goodbye!");
  }
}