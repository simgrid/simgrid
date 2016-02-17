/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package tracing;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.trace.Trace;

public class Sender extends Process {
  private final double commSizeLat = 1;
  private final double commSizeBw = 100000000;

  public Sender(Host host, String name, String[] args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("hello !"); 
    Trace.hostPushState (getHost().getName(), "PM_STATE", "sendingPing");

    int hostCount = args.length;
    Msg.info("host count: " + hostCount);
    String mailboxes[] = new String[hostCount]; 
    double time;
    double computeDuration = 0;
    PingPongTask ping, pong;

    for(int pos = 0; pos < args.length ; pos++) {
      try {
        mailboxes[pos] = Host.getByName(args[pos]).getName();
      } catch (HostNotFoundException e) {
        Msg.info("Invalid deployment file: " + e.toString());
        System.exit(1);
      }
    }

    for (int pos = 0; pos < hostCount; pos++) { 
      time = Msg.getClock(); 
      Msg.info("sender time: " + time);
      ping = new PingPongTask("no name",computeDuration,commSizeLat);
      ping.setTime(time);
      ping.send(mailboxes[pos]);

      Trace.hostPushState (getHost().getName(), "PM_STATE", "waitingPong");
      pong = (PingPongTask)Task.receive(getHost().getName());
      double timeGot = Msg.getClock();
      double timeSent = ping.getTime();
      double communicationTime=0;

      Msg.info("Got at time "+ timeGot);
      Msg.info("Was sent at time "+timeSent);
      time=timeSent;

      communicationTime=timeGot - time;
      Msg.info("Communication time : " + communicationTime);

      Msg.info(" --- bw "+ commSizeBw/communicationTime + " ----");

      /* Pop the last state (going back to sending ping) */  
      Trace.hostPopState (getHost().getName(), "PM_STATE");
    }

    /* Pop the sendingPong state */  
    Trace.hostPopState (getHost().getName(), "PM_STATE");
    Msg.info("goodbye!");
  }
}
