	/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.pingpong;
import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;

public class Sender extends Process {
  private static final double COMM_SIZE_LAT = 1;

  public Sender(String hostname, String name, String[] args) throws HostNotFoundException {
    super(hostname,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("hello!");

    int hostCount = args.length;

    Msg.info("host count: " + hostCount);
    String[] mailboxes = new String[hostCount]; 
    double time;
    double computeDuration = 0;
    PingPongTask task;

    for(int pos = 0; pos < args.length ; pos++) {
      try {
        mailboxes[pos] = Host.getByName(args[pos]).getName();
      } catch (HostNotFoundException e) {
        e.printStackTrace();
        Msg.info("Invalid deployment file: " + e.toString());
      }
    }

    for (int pos = 0; pos < hostCount; pos++) { 
      time = Msg.getClock(); 

      Msg.info("sender time: " + time);

      task = new PingPongTask("no name",computeDuration,COMM_SIZE_LAT);
      task.setTime(time);

      task.send(mailboxes[pos]);
    }

    Msg.info("goodbye!");
  }
}