/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** This exception is raised when looking for a non-existing host. */
public class HostNotFoundException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>HostNotFoundException</code> without a detail message. */ 
	public HostNotFoundException() {
		super();
	}
	/** Constructs an <code>HostNotFoundException</code> with a detail message. */
	public HostNotFoundException(String s) {
		super(s);
	}
}
