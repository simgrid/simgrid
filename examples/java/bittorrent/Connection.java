/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package bittorrent;
import java.util.Arrays;

public class Connection {
  public int id;
  public char bitfield[];
  public String mailbox;
  // Indicates if we are interested in something this peer has
  public boolean amInterested = false;
  // Indicates if the peer is interested in one of our pieces
  public boolean interested = false;
  // Indicates if the peer is choked for the current peer
  public boolean chokedUpload = true;
  // Indicates if the peer has choked the current peer
  public boolean chokedDownload = true;
  // Number of messages we have received from the peer
  public int messagesCount = 0;
  public double peerSpeed = 0;
  public double lastUnchoke = 0;

  public Connection(int id) {
    this.id = id;
    this.mailbox = Integer.toString(id);
  }

  // Add a new value to the peer speed average
  public void addSpeedValue(double speed) {
    peerSpeed = peerSpeed * 0.55 + speed * 0.45;
    // peerSpeed = (peerSpeed * messagesCount + speed) / (++messagesCount);    
  }

  @Override
  public String toString() {
    return "Connection [id=" + id + ", bitfield=" + Arrays.toString(bitfield) + ", mailbox=" + mailbox
        + ", amInterested=" + amInterested + ", interested=" + interested + ", chokedUpload=" + chokedUpload
        + ", chokedDownload=" + chokedDownload + "]";
  }
}
