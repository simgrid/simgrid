/* Copyright (c) 2006-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package dht.chord;

public class GetPredecessorAnswerTask extends ChordTask {
  private int answerId;

  public GetPredecessorAnswerTask(String issuerHostname, String answerTo, int answerId) {
    super(issuerHostname,answerTo);
    this.answerId = answerId;
  }

  public int getAnswerId(){
    return this.answerId;
  }
}
