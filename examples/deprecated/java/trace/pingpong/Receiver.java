/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package trace.pingpong;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;
import org.simgrid.trace.Trace;

public class Receiver extends Process {
  private static final double COMM_SIZE_LAT = 1;
  private static final double COMM_SIZE_BW = 100000000;
  private static final String PM_STATE = Main.PM_STATE;

  public Receiver(String hostname, String name, String[]args) throws HostNotFoundException {
    super(hostname,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("hello!");
    Trace.hostPushState (getHost().getName(), PM_STATE, "waitingPing");

    /* Wait for the ping */ 
    Msg.info("try to get a task");

    PingPongTask ping = (PingPongTask)Task.receive(getHost().getName());
    double timeGot = Msg.getClock();
    double timeSent = ping.getTime();

    Msg.info("Got at time "+ timeGot);
    Msg.info("Was sent at time "+timeSent);
    double time=timeSent;

    double communicationTime=timeGot - time;
    Msg.info("Communication time : " + communicationTime);

    Msg.info(" --- bw "+ COMM_SIZE_BW/communicationTime + " ----");

    /* Send the pong */
    Trace.hostPushState (getHost().getName(), PM_STATE, "sendingPong");
    double computeDuration = 0;
    PingPongTask pong = new PingPongTask("no name",computeDuration,COMM_SIZE_LAT);
    pong.setTime(time);
    pong.send(ping.getSource().getName());

    /* Pop the two states */
    Trace.hostPopState (getHost().getName(), PM_STATE);
    Trace.hostPopState (getHost().getName(), PM_STATE);

    Msg.info("goodbye!");
  }
}
