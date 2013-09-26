/*
 * This exception is raised when looking for a non-existing process.
 *
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */

package org.simgrid.msg;

/**
 * This exception is raised when looking for a non-existing process.
 */
public class ProcessNotFoundException extends MsgException {
	private static final long serialVersionUID = 1L;

	/**
	 * Constructs an <code>ProcessNotFoundException</code> without a 
	 * detail message. 
	 */
	public ProcessNotFoundException() {
		super();
	}
	/**
	 * Constructs an <code>ProcessNotFoundException</code> with a detail message. 
	 *
	 * @param   s   the detail message.
	 */
	public ProcessNotFoundException(String s) {
		super(s);
	}
}
