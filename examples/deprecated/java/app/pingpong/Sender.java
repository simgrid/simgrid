/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

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
		Msg.info("Host count: " + args.length);

		for (int i = 0 ; i<Main.TASK_COUNT; i++) {

			for(int pos = 0; pos < args.length ; pos++) {
				String hostname = Host.getByName(args[pos]).getName(); // Make sure that this host exists

				double time = Msg.getClock(); 
				Msg.info("sender time: " + time);

				PingPongTask task = new PingPongTask("no name", /* Duration: 0 flops */ 0, COMM_SIZE_LAT, time);
				task.send(hostname);
			}
		}
		Msg.info("Done.");
	}
}