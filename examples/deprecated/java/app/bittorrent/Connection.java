/* Copyright (c) 2006-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package app.bittorrent;
import java.util.Arrays;

public class Connection {
  protected int id;
  protected char[] bitfield;
  protected String mailbox;
  // Indicates if we are interested in something this peer has
  protected boolean amInterested = false;
  // Indicates if the peer is interested in one of our pieces
  protected boolean interested = false;
  // Indicates if the peer is choked for the current peer
  protected boolean chokedUpload = true;
  // Indicates if the peer has choked the current peer
  protected boolean chokedDownload = true;
  // Number of messages we have received from the peer
  protected int messagesCount = 0;
  protected double peerSpeed = 0;
  protected double lastUnchoke = 0;

  public Connection(int id) {
    this.id = id;
    this.mailbox = Integer.toString(id);
  }

  // Add a new value to the peer speed average
  public void addSpeedValue(double speed) {
    peerSpeed = peerSpeed * 0.55 + speed * 0.45;
  }

  @Override
  public String toString() {
    return "Connection [id=" + id + ", bitfield=" + Arrays.toString(bitfield) + ", mailbox=" + mailbox
        + ", amInterested=" + amInterested + ", interested=" + interested + ", chokedUpload=" + chokedUpload
        + ", chokedDownload=" + chokedDownload + "]";
  }
}
