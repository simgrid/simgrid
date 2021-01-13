/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package dht.chord;

public class FindSuccessorTask extends ChordTask {
  private int requestId;

  public FindSuccessorTask(String issuerHostname, String answerTo,  int requestId) {
    super(issuerHostname, answerTo);
    this.requestId = requestId;
  }

  public int getRequestId(){
    return this.requestId;
  }
}
