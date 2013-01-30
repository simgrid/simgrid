/*
 * Copyright 2006-2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package bittorrent;

import org.simgrid.msg.Task;
/**
 * Tasks sent between peers
 */
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
	/**
	 * Constructor, builds a value-less message
	 * @param type
	 * @param issuerHostname
	 * @param mailbox
	 * @param peerId
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId) {
		this(type,issuerHostname,mailbox,peerId,-1,false,-1,-1);
	}
	/**
	 * Constructor, builds a new "have/request/piece" message
	 * @param type
	 * @param issuerHostname
	 * @param mailbox
	 * @param peerId
	 * @param index
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index) {
		this(type,issuerHostname,mailbox,peerId,index,false,-1,-1);
	}
	/**
	 * Constructor, builds a new bitfield message
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, char bitfield[]) {
		this(type,issuerHostname,mailbox,peerId,-1,false,-1,-1);
		this.bitfield = bitfield;
	}
	/**
	 * Constructor, build a new "request"  message
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index, int blockIndex, int blockLength) {
		this(type,issuerHostname,mailbox,peerId,index,false,blockIndex,blockLength);
	}
	/**
	 * Constructor, build a new "piece" message
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index, boolean stalled, int blockIndex, int blockLength) {
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
