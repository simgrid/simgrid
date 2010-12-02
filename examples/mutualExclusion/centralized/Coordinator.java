/*
 * $Id$
 *
 * Copyright 2010. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import java.util.LinkedList;

import simgrid.msg.Msg;
import simgrid.msg.MsgException;
import simgrid.msg.Task;

public class Coordinator extends simgrid.msg.Process  {

	LinkedList<RequestTask> waitingQueue=new LinkedList<RequestTask>();
	int CsToServe;
		
	public void main(String[] args) throws MsgException {
		CsToServe = Integer.parseInt(args[0]);
		Task task;
		while (CsToServe >0) {
			task = Task.receive("coordinator");
			if (task instanceof RequestTask) {
				RequestTask t = (RequestTask) task;
				if (waitingQueue.isEmpty()) {
				   Msg.info("Got a request from "+t.from+". Queue empty: grant it");
					GrantTask tosend =  new GrantTask();
					tosend.send(t.from);
				} else {
					waitingQueue.push(t);
				}
			} else if (task instanceof ReleaseTask) {
				if (!waitingQueue.isEmpty()) {
					RequestTask req = waitingQueue.removeLast();
					GrantTask tosend = new GrantTask();
					tosend.send(req.from);
				}
				CsToServe--;
				if (waitingQueue.isEmpty() && CsToServe==0) {
					Msg.info("we should shutdown the simulation now");
				}
			}
		}
	}
}