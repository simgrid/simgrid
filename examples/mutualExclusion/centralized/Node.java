/*
 * $Id$
 *
 * Copyright 2010. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.MsgException;
import simgrid.msg.Task;
import simgrid.msg.Msg;

public class Node extends simgrid.msg.Process {

	public void request(double CStime) throws MsgException {
		RequestTask req = new RequestTask(this.name);
	   Msg.info("Send a request to the coordinator");
		req.send("coordinator");
	   Msg.info("Wait for a grant from the coordinator");
		Task.receive(this.name); // FIXME: ensure that this is a grant
		Task compute = new Task("CS", CStime, 0);
		compute.execute();
		ReleaseTask release = new ReleaseTask();
		release.send("coordinator");
	}
	
	public void main(String[] args) throws MsgException {
		request(Double.parseDouble(args[1]));
	}
}