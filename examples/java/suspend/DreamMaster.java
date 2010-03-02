/*
 * $Id$
 *
 * Copyright 2006,2007,2010. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class DreamMaster extends simgrid.msg.Process {
    
   public void main(String[] args) throws NativeException {
      Msg.info("Hello !");
      Msg.info("Let's create a lazy guy.");
       
      try {
	 Host currentHost = Host.currentHost();
	 Msg.info("Current hostname: " + currentHost.getName());
	 
	 LazyGuy lazy = new LazyGuy(currentHost,"LazyGuy");
	 Msg.info("Let's wait a little bit...");

	 simgrid.msg.Process.waitFor(10.0);

	 Msg.info("Let's wake the lazy guy up! >:) ");
	 lazy.restart();
	 
    	} catch(HostNotFoundException e) {
	   System.err.println(e);
	   System.exit(1);
	}
       
      //lazy.migrate(currentHost);
	 
      Msg.info("OK, goodbye now.");
   }
}