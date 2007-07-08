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

public class PingPongTask extends Task {
   
   private double timeVal;
   
   public PingPongTask() throws JniException, NativeException {
      this.timeVal = 0;
   }
   
   public PingPongTask(String name, double computeDuration, double messageSize) throws JniException, NativeException {      
      super(name,computeDuration,messageSize);		
   }
   
   public void setTime(double timeVal){
      this.timeVal = timeVal;
   }
   
   public double getTime() {
      return this.timeVal;
   }
}
    