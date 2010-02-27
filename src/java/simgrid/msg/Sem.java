/*
 * Simple semaphore implementation, from Doug Lea (public domain)
 *
 * Copyright 2006,2007,2010 The SimGrid Team           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */  
package simgrid.msg;
public class Sem {
	/******************************************************************/ 
	/* Simple semaphore implementation, from Doug Lea (public domain) */ 
	/******************************************************************/ 
	private int permits_;
	public Sem(int i) {		permits_ = i;	} 	public void acquire() throws InterruptedException {
		if (Thread.interrupted())
			throw new InterruptedException();
		synchronized(this) {
			try {
				while (permits_ <= 0)
					wait();
				--permits_;
			}
			catch(InterruptedException ex) {
				notify();
				throw ex;
			}
		}
	}
	public synchronized void release() {
		++(this.permits_);
		notify();
	} } 
