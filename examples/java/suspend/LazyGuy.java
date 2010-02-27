/*
 * $Id$
 *
 * Copyright 2006,2007,2010. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class LazyGuy extends simgrid.msg.Process {
   public LazyGuy() {
      super();
    }
   
   public LazyGuy(Host host,String name) 
     throws NullPointerException, HostNotFoundException, JniException, NativeException
     {
	super(host,name,null);
     }
	
	 
   public void main(String[] args) throws JniException, NativeException {
      Msg.info("Hello !");
      
      Msg.info("Nobody's watching me ? Let's go to sleep.");
      pause();
      
      Msg.info("Uuuh ? Did somebody call me ?");
      Msg.info("Mmmh, goodbye now."); 
   }
}