/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.pingpong;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

public class Receiver extends Process {
	private static final double COMM_SIZE_BW = 100000000;
	public Receiver(String hostname, String name, String[]args) throws HostNotFoundException {
		super(hostname, name, args);
	}

	public void main(String[] args) throws MsgException {
		for (int i = 0 ; i < Main.TASK_COUNT; i++) {
			Msg.info("Wait for a task");

			PingPongTask task = (PingPongTask)Task.receive(getHost().getName());
			double timeGot = Msg.getClock();
			double timeSent = task.getTime();

			Msg.info("Got one that was sent at time "+ timeSent);

			double communicationTime = timeGot - timeSent;
			Msg.info("Communication time : " + communicationTime);
			Msg.info(" --- bw "+ COMM_SIZE_BW/communicationTime + " ----");
		}
		Msg.info("Done.");
	}
}