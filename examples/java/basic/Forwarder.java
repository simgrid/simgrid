/*
 * $Id$
 *
 * Copyright 2006,2007,2010 The SimGrid Team
 * All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Forwarder extends simgrid.msg.Process {
    
   public void main(String[] args) throws JniException, NativeException {
      if (args.length < 3) {	 
	 Msg.info("Forwarder needs 3 arguments (input mailbox, first output mailbox, last one)");
	 Msg.info("Got "+args.length+" instead");
	 System.exit(1);
      }
      int input = Integer.valueOf(args[0]).intValue();		
      int firstOutput = Integer.valueOf(args[1]).intValue();		
      int lastOutput = Integer.valueOf(args[2]).intValue();		
      
      int taskCount = 0;
      int slavesCount = lastOutput - firstOutput + 1;
      Msg.info("Receiving on 'slave_"+input+"'");
      while(true) {
	 Task t = Task.receive("slave_"+input);	
	 
	 if (t instanceof FinalizeTask) {
	    Msg.info("Got a finalize task. Let's forward that we're done.");
	    
	    for (int cpt = firstOutput; cpt<=lastOutput; cpt++) {
	       Task tf = new FinalizeTask();
	       tf.send("slave_"+cpt);
	    }
	    break;
	 }
	 BasicTask task = (BasicTask)t;
	 int dest = firstOutput + (taskCount % slavesCount);
	 
	 Msg.info("Sending \"" + task.getName() + "\" to \"slave_" + dest + "\"");
	 task.send("slave_"+dest);
	    
	 taskCount++;
      }
      
	 
      Msg.info("I'm done. See you!");
   }
}

