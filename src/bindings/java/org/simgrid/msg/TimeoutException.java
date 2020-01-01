/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** Exception raised when sending a task timeouts */
public class TimeoutException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>TimeoutFailureException</code> without a detail message. */
	public TimeoutException() {
		super();
	}
	/** Constructs an <code>TransferFailureException</code> with a detail message. */
	public TimeoutException(String s) {
		super(s);
	}
}
