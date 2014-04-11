/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package surfCpuModel;
import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

public class Sender extends Process {
	public Sender(Host host, String name, String[] args) {
		super(host,name,args);
	}
    private final double commSizeLat = 1;
    final double commSizeBw = 100000000;

    public void main(String[] args) throws MsgException {

       Msg.info("helloo!");

       String receiverName = args[0];
       double oldTime, curTime;
       double computeDuration = 10E8;
       Task task;

       oldTime = Msg.getClock();
	     task = new Task("no name",computeDuration,commSizeLat);
	     task.send(receiverName);
       curTime = Msg.getClock();
       Msg.info("Send duration: " + (curTime - oldTime));

       Msg.info("goodbye!");
    }
}
