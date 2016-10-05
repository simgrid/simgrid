/* This exception is an abstract class grouping all MSG-related exceptions */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** This exception is an abstract class grouping all MSG-related exceptions */
public abstract class MsgException extends Exception {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>MsgException</code> without a detail message. */
	public MsgException() {
		super();
	}
	/** Constructs an <code>MsgException</code> with a detail message. */ 
	public MsgException(String msg) {
		super(msg);
	}
}
