/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;

/**
 * @brief "PING" task sent by a node to another to see if it is still alive
 */
public class PingTask extends KademliaTask {
	/**
	 * Constructor
	 */
	public PingTask(int senderId) {
		super(senderId);
	}
}
