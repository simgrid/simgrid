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
      
      while(true) {
	 double time1 = Msg.getClock();       
	 Task t = Task.get(0);	
 
	 if (t instanceof FinalizeTask) {
	    break;
	 }
	 CommTimeTask task = (CommTimeTask)t;
	     
	 if(time1 < task.getTime())
	   time1 = task.getTime();
	 
/*	 double time2 = Msg.getClock();
	 Msg.info("Processing \"" + task.getName() + "\" " + getHost().getName() + 
		  " (Communication time : " +  (time2 - time1) + ")");
*/	     
	 task.execute();
      }
       
      Msg.info("Received Finalize. I'm done. See you!");
   }
}