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

public class Sender extends simgrid.msg.Process {
  
    private final double commSizeLat = 1;
    final double commSizeBw = 100000000;
   
    public void main(String[] args) throws JniException, NativeException {
    	
       Msg.info("hello!");
        
       int hostCount = args.length;
        
       Msg.info("host count : " + hostCount);
       Host hostTable[] = new Host[hostCount]; 
       double time;
       double computeDuration = 0;
       PingPongTask task;
        
       for(int pos = 0; pos < args.length ; pos++) {
	  try {
	     hostTable[pos] = Host.getByName(args[pos]);
	  } catch (HostNotFoundException e) {
	     Msg.info("Invalid deployment file: " + e.toString());	     
	     System.exit(1);
	  }
        }
        
        Channel channel = new Channel(0);
        
        for (int pos = 0; pos < hostCount; pos++) { 
	   time = Msg.getClock(); 
            
	   Msg.info("sender time : " + time);
	   
	   task = new PingPongTask("no name",computeDuration,commSizeLat);
	   task.setTime(time);
            
	   channel.put(task, hostTable[pos]); 
        } 
        
        Msg.info("goodbye!");
    }
}