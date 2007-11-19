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

public class Forwarder extends simgrid.msg.Process {
    
   public void main(String[] args) throws JniException, NativeException {
      Msg.info("hello!");
      
      int slavesCount = args.length;
      Host[] slaves = new Host[slavesCount];
      
      for (int i = 0; i < args.length; i++) {
	 try {	      
	    slaves[i] = Host.getByName(args[i]);
	 } catch (HostNotFoundException e) {
	    Msg.info("Buggy deployment file");
	    e.printStackTrace();
	    System.exit(1);
	 }
      }
      
      int taskCount = 0;
      while(true) {
	 Task t = Task.get(0);	
	 
	 if (t instanceof FinalizeTask) {
	    Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	    
	    for (int cpt = 0; cpt<slavesCount; cpt++) {
	       slaves[cpt].put(0,new FinalizeTask());
	    }
	    break;
	 }
	 BasicTask task = (BasicTask)t;
	 
	 Msg.info("Received \"" + task.getName() + "\" ");
	 	    
	 Msg.info("Sending \"" + task.getName() + "\" to \"" + slaves[taskCount % slavesCount].getName() + "\"");
	 slaves[taskCount % slavesCount].put(0, task);
	    
	 taskCount++;
      }
      
	 
      Msg.info("I'm done. See you!");
   }
}

