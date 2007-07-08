/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier         
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class DreamMaster extends simgrid.msg.Process {
    
    public void main(String[] args) throws JniException, NativeException {
       Msg.info("Hello !");
       
       Msg.info("Let's create a lazy guy.");
       
       Host currentHost = Host.currentHost();
       Msg.info("Current host  name : " + currentHost.getName());
	  	
       LazyGuy lazy = new LazyGuy();
//       lazy.migrate(currentHost);
	 
       Msg.info("Let's wait a little bit...");
       simgrid.msg.Process.waitFor(10.0);
       Msg.info("Let's wake the lazy guy up! >:) ");
       lazy.restart();
	  	
       Msg.info("OK, goodbye now.");
    }
}