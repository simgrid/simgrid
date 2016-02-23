/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;

/**
 * @brief Find node tasks sent by a node to another "Find Node" task sent by a node to another. Ask him for its closest
 * nodes from a destination.
 */
public class FindNodeTask extends KademliaTask {
  /* Id of the node we are trying to find: the destination */
  private int destination;

  public FindNodeTask(int senderId, int destination) {
    super(senderId);  
    this.destination = destination;
  }

  public int getDestination() {
    return destination;
  }
}
