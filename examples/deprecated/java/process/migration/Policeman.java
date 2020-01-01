/* Copyright (c) 2012-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.migration;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;

public class Policeman extends Process {
  public Policeman(String hostname, String name) throws HostNotFoundException {
    super(hostname, name);
  }

  public void main(String[] args) throws MsgException {
    waitFor(1);

    Msg.info("Wait a bit before migrating the emigrant.");

    Main.mutex.acquire();

    Main.processToMigrate.migrate(Host.getByName("Jacquelin"));
    Msg.info("I moved the emigrant");
    Main.processToMigrate.resume();
  }
}