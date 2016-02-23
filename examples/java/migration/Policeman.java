/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package migration;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Policeman extends Process {
  public Policeman(Host host, String name, String[]args) {
    super(host,name,args);
  }

  @Override
  public void main(String[] args) throws MsgException {
    waitFor(1);
    
    Msg.info("Wait a bit before migrating the emigrant.");
    
    Migration.mutex.acquire();
    
    Migration.processToMigrate.migrate(Host.getByName("Jacquelin"));
    Msg.info("I moved the emigrant");
    Migration.processToMigrate.resume();
  }
}