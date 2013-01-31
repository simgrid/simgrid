/*
 * Copyright 2006-2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package chord;

import org.simgrid.msg.Task;

import chord.Common;
/**
 * Base class for all Tasks in Chord.
 */
public class ChordTask extends Task {
	public String issuerHostName;
	public String answerTo;
	public ChordTask() {
		this(null,null);
	}
	public ChordTask(String issuerHostName, String answerTo) {
		super(null, Common.COMP_SIZE, Common.COMM_SIZE);
		this.issuerHostName = issuerHostName;
		this.answerTo = answerTo;
	}
}
