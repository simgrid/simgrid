/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package pingPong;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;

public class Sender extends Process {
  private final double commSizeLat = 1;
  final double commSizeBw = 100000000;

  public Sender(Host host, String name, String[] args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("hello!");

    int hostCount = args.length;

    Msg.info("host count: " + hostCount);
    String mailboxes[] = new String[hostCount]; 
    double time;
    double computeDuration = 0;
    PingPongTask task;

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

      task = new PingPongTask("no name",computeDuration,commSizeLat);
      task.setTime(time);

      task.send(mailboxes[pos]);
    }

    Msg.info("goodbye!");
  }
}