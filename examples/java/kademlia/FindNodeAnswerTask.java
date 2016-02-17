/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kademlia;
import kademlia.Answer;

public class FindNodeAnswerTask extends KademliaTask {
  protected int destinationId;
  protected Answer answer;

  public FindNodeAnswerTask(int senderId, int destinationId, Answer answer) {
    super(senderId);
    this.destinationId = destinationId;
    this.answer = answer;
  }
  public int getDestinationId() {
    return destinationId;
  }
  public Answer getAnswer() {
    return answer;
  }
}
