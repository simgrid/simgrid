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

public class Slave extends simgrid.msg.Process {
   
   public void main(String[] args) throws JniException, NativeException {
      
      Msg.info("Hello i'm a slave");
      
      Channel channel = new Channel(0);
		
      while(true) {
	 int a;
	 double time1 = Msg.getClock();       
	 Task t = channel.get();	
	 double time2 = Msg.getClock();
 
	 if (t instanceof FinalizeTask) {
	    Msg.info("Received Finalize " + getHost().getName());
	    break;
	 }
	 CommTimeTask task = (CommTimeTask)t;
	     
	 if(time1 < task.getTime())
	   time1 = task.getTime();
	 
/*	 Msg.info("Processing \"" + task.getName() + "\" " + getHost().getName() + 
		  " (Communication time : " +  (time2 - time1) + ")");
*/	     
	 task.execute();
	 
	 
      }
       
      Msg.info("I'm done. See you!" + getHost().getName());
   }
}