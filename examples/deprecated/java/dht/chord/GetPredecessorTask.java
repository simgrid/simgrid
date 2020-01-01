/* Copyright (c) 2006-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package dht.chord;

public class GetPredecessorTask extends ChordTask {
  public GetPredecessorTask(String issuerHostName, String answerTo) {
    super(issuerHostName, answerTo);
  }
}
