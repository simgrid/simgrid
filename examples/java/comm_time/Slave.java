/*
 * Copyright 2006,2007,2010. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

import simgrid.msg.*;

public class Slave extends simgrid.msg.Process {
   public void main(String[] args) throws NativeException {
      if (args.length < 1) {
	 Msg.info("Slave needs 1 argument (its number)");
	 System.exit(1);
      }

      int num = Integer.valueOf(args[0]).intValue();
      Msg.info("Receiving on 'slave_"+num+"'");
      
      while(true) { 
	 Task task = Task.receive("slave_"+num);	
	 
	 if (task instanceof FinalizeTask) {
	    break;
	 }
	 task.execute();
       }
       
      Msg.info("Received Finalize. I'm done. See you!");
    }
}