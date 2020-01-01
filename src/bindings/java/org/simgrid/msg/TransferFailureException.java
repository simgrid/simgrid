/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.msg;

/** Exception raised if sending a task fails */
public class TransferFailureException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>TransferFailureException</code> without a detail message. */
	public TransferFailureException() {
		super();
	}
	/**
	 * Constructs an <code>TransferFailureException</code> with a detail message.
	 *
	 * @param   s   the detail message.
	 */
	public TransferFailureException(String s) {
		super(s);
	}
}
