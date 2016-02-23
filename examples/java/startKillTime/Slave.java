/* Copyright (c) 2006-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package startKillTime;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

/* Lazy Guy Slave, suspends itself ASAP */
public class Slave extends Process {
  public Slave(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("Hello!");
    try {
      waitFor(10.0);
      Msg.info("OK, goodbye now.");
    } catch (MsgException e) {
      Msg.debug("Wait cancelled.");
    }
  }
}
