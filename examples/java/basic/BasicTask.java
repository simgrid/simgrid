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

public class BasicTask extends Task {
    	
    private int data;
    	
    public BasicTask(String name, double computeDuration, double messageSize) throws JniException{
	super(name,computeDuration,messageSize);		
    }
	
    public BasicTask() {
	this.data = 0;
    }
    	
    public void setData(int data){
	this.data = data;
    }
    	
    public int getData() {
	return this.data;
    }
}
