/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.tokenring;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

public class RelayRunner extends Process {

	private static final int TASK_COMM_SIZE = 1000000; /* The token is 1MB long*/

	public RelayRunner(Host host, String name, String[]args) {
		super(host,name,args);
	}

	/* This is the function executed by this kind of processes */
	@Override
	public void main(String[] args) throws MsgException {
		// In this example, the processes are given numerical names: "0", "1", "2", and so on 
		int rank = Integer.parseInt(this.getName());

		if (rank == 0) {
			/* The root (rank 0) first sends the token then waits to receive it back */
			
			String mailbox = "1"; 
			Task token = new Task("Token", 0/* no computation associated*/ , TASK_COMM_SIZE );

			Msg.info("Host '"+rank+"' send '"+token.getName()+"' to Host '"+mailbox+"'");
			token.send(mailbox);
			
			token = Task.receive(this.getName()); // Get a message from the mailbox having the same name as the current processor
			
			Msg.info("Host '"+rank+"' received '"+token.getName()+"'");

		} else {
		    /* The others processes receive on their name (coming from their left neighbor -- rank-1)
		     * and send to their right neighbor (rank+1) */
			Task token = Task.receive(this.getName());
			
		    Msg.info("Host '"+rank+"' received '"+token.getName()+"'");

		    String mailbox = Integer.toString(rank+1);
		    if (rank+1 == Host.getCount()) {
		    	/* The last process has no right neighbor, so it sends the token back to rank 0 */
		    	mailbox = "0";
		    }
		    
		    Msg.info("Host '"+rank+"' send '"+token.getName()+"' to Host '"+mailbox+"'");
		    token.send(mailbox);
		}
	}
}
