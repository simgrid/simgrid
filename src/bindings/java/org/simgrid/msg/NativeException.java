/* This exception is raised when there is an error within the C world of SimGrid. */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** This exception is raised when there is an error within the C world of SimGrid. */
public class NativeException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>NativeException</code> with a detail message. */
	public NativeException(String s) {
		super(s);
	}
}
