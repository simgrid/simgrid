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
      
      Channel channel = new Channel(0);
		
      int taskCount = 0;
      while(true) {
			
	 BasicTask task = (BasicTask)channel.get();
	 
	 Msg.info("Received \"" + task.getName() + "\" ");
	 
	 if(task.getData() == 221297) {
	    Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.");
	    
	    for (int cpt = 0; cpt<slavesCount; cpt++) {
	       BasicTask forwardedtask = new BasicTask("finalize", 0, 0);
	       forwardedtask.setData(221297);
	       
	       channel.put(forwardedtask,slaves[cpt]);
	       break;
	    }
	    
	 } else {
	    
	    Msg.info("Sending \"" + task.getName() + "\" to \"" + slaves[taskCount % slavesCount].getName() + "\"");
	    channel.put(task, slaves[taskCount % slavesCount]);
	    
	    taskCount++;
	 }
	 
	 Msg.info("I'm done. See you!");
      }
   }
}

