/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** Exception raised when a task is cancelled. */
public class TaskCancelledException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>TaskCancelledException</code> without a detail message. */
	public TaskCancelledException() {
		super();
	}
	/** Constructs an <code>TaskCancelledException</code> with a detail message. */
	public TaskCancelledException(String s) {
		super(s);
	}
}
