/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package bittorrent;
import org.simgrid.msg.Task;

public class MessageTask extends Task {
  public enum Type {
    HANDSHAKE,
    CHOKE,
    UNCHOKE,
    INTERESTED,
    NOTINTERESTED,
    HAVE,
    BITFIELD,
    REQUEST,
    PIECE
  };

  public Type type;
  public String issuerHostname;
  public String mailbox;
  public int peerId;
  public char bitfield[];
  public int index;
  public int blockIndex;
  public int blockLength;
  public boolean stalled;

  public MessageTask(Type type, String issuerHostname, String mailbox, int peerId) {
    this(type,issuerHostname,mailbox,peerId,-1,false,-1,-1);
  }

  public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index) {
    this(type,issuerHostname,mailbox,peerId,index,false,-1,-1);
  }

  // builds a new bitfield message
  public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, char bitfield[]) {
    this(type,issuerHostname,mailbox,peerId,-1,false,-1,-1);
    this.bitfield = bitfield;
  }

  // build a new "request"  message
  public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index, int blockIndex,
                     int blockLength) {
    this(type,issuerHostname,mailbox,peerId,index,false,blockIndex,blockLength);
  }

  // build a new "piece" message
  public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index, boolean stalled,
                     int blockIndex, int blockLength) {
    this.type = type;
    this.issuerHostname = issuerHostname;
    this.mailbox = mailbox;
    this.peerId = peerId;
    this.index = index;
    this.stalled = stalled;
    this.blockIndex = blockIndex;
    this.blockLength = blockLength;
  }
}
