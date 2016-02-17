/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package bittorrent;
import java.util.ArrayList;

import org.simgrid.msg.Task;

/* Task exchanged between the tracker and the peers. */
public class TrackerTask extends Task {
  public enum Type {
    REQUEST,
    ANSWER
  };

  public Type type;
  public String hostname;
  public String mailbox;
  public int peerId;
  public int uploaded;
  public int downloaded;
  public int left;
  public double interval;
  public ArrayList<Integer> peers;

  public TrackerTask(String hostname, String mailbox, int peerId) {
    this(hostname, mailbox, peerId, 0, 0, Common.FILE_SIZE);
  }

  public TrackerTask(String hostname, String mailbox, int peerId, int uploaded, int downloaded, int left) {
    super("", 0, Common.TRACKER_COMM_SIZE);
    this.type = Type.REQUEST;
    this.hostname = hostname;
    this.mailbox = mailbox;
    this.peerId = peerId;
    this.uploaded = uploaded;
    this.downloaded = downloaded;
    this.left = left;
    this.peers = new ArrayList<Integer>();
  }
}
