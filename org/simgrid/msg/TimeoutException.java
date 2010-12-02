/*
 * This exception is raised when looking for a non-existing host.
 *
 * Copyright 2006,2007,2010 The SimGrid Team           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */

package simgrid.msg;

/**
 * This exception is raised when time's out while sending tasks.
 */
public class TimeoutException extends MsgException {
	private static final long serialVersionUID = 1L;

	/** Constructs an <code>TimeoutFailureException</code> without a detail message. */ 
	public TimeoutException() {
		super();
	}
	/**
	 * Constructs an <code>TransferFailureException</code> with a detail message. 
	 *
	 * @param   s   the detail message.
	 */
	public TimeoutException(String s) {
		super(s);
	}
}
