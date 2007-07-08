/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 * (GNU LGPL) which comes with this package.
 */
 
package simgrid.msg;

public class JniException extends MsgException {
	
   private static final long serialVersionUID = 1L;

   /**
    * Constructs an <code>JniException</code> without a 
    * detail message. 
    */
   public JniException() {
      super();
   }
   
   /**
    * Constructs an <code>JniException</code> with a detail message. 
    *
    * @param   s   the detail message.
    */
   public JniException(String s) {
      super(s);
   }
}