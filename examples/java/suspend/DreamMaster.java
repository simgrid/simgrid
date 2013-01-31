/*
 * Copyright 2006-2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package suspend;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
public class DreamMaster extends Process {
	public DreamMaster(Host host, String name, String[]args) {
		super(host,name,args);
	} 
	public void main(String[] args) throws MsgException {
		Msg.info("Let's create a lazy guy.");
		Process lazyGuy = new LazyGuy(getHost(),"Lazy",null);
		lazyGuy.start();
		Msg.info("Let's wait a little bit...");
		waitFor(10);
		Msg.info("Let's wake the lazy guy up! >:) BOOOOOUUUHHH!!!!");
		lazyGuy.resume();
		Msg.info("OK, goodbye now.");
	}
}