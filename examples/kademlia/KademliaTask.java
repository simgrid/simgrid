/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;

import kademlia.Common;

import org.simgrid.msg.Task;

/**
 * @brief Base class for all the tasks related to Kademlia.
 */
public class KademliaTask extends Task {
	/**
	 * Sender id
	 */
	protected int senderId;
	
	/**
	 * Constructor
	 */
	public KademliaTask(int senderId) {
		super("kademliatask",Common.COMP_SIZE,Common.COMM_SIZE);
		this.senderId = senderId;
	}
	public int getSenderId() {
		return senderId;
	}
}
