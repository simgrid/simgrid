/*
 * Copyright 2006-2012. The SimGrid Team. All rights reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package startKillTime;
import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;

/**
 * Lazy Guy Slave, suspends itself ASAP
 */
public class Slave extends Process {
	public Slave(Host host, String name, String[]args) {
		super(host,name,args);
	} 
	public void main(String[] args) throws TransferFailureException, HostFailureException, TimeoutException {
		Msg.info("Hello!");
                try {
                        waitFor(10.0);
                        Msg.info("OK, goodbye now.");
                } catch (MsgException e) {
                        Msg.debug("Wait cancelled.");
                }
	
  }
}
