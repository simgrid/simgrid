/*
 * java.com.simgrid.msg.MsgException.java	1.00 07/05/01
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */
 
package simgrid.msg;

import java.lang.Exception;

public class MsgException extends Exception {
	
	private static final long serialVersionUID = 1L;

    /**
     * Constructs an <code>MsgException</code> without a 
     * detail message. 
     */
    public MsgException() {
	super();
    }

    /**
     * Constructs an <code>MsgException</code> with a detail message. 
     *
     * @param   s   the detail message.
     */
    public MsgException(String s) {
	super(s);
    }
}