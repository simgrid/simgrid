/*
 * This exception is raised when there is an error within the C world of SimGrid.
 *
 * Copyright 2006,2007,2010 The SimGrid team
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */
package simgrid.msg;

/**
 * This exception is raised when there is an error within the C world of SimGrid. 
 * Refer to the string message for more info on what went wrong.
 */
public class NativeException extends MsgException {
	private static final long serialVersionUID = 1L;

	/**
	 * Constructs an <code>NativeException</code> with a detail message. 
	 *
	 * @param   s   the detail message.
	 */
	public NativeException(String s) {
		super(s);
	}
}
