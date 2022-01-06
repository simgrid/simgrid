/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.kill;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;

public class Killer extends Process {
  public Killer(String hostname, String name) throws HostNotFoundException {
    super(hostname, name);
  }
  public void main(String[] args) throws MsgException {
    Victim poorVictim = null;
    Msg.info("Hello!");
    try {
      poorVictim = new Victim("Boivin","victim");
      poorVictim.start();
    } catch (MsgException e){
      e.printStackTrace();
      Msg.error("Cannot create the victim process!");
      return;
    }
    sleep(10000);
    Msg.info("Resume Process");
    poorVictim.resume();
    sleep(1000);
    Msg.info("Kill Process");
    poorVictim.kill();

    Msg.info("Ok, goodbye now.");
    // The actor can also commit a suicide with the following command
    exit(); // This will forcefully stop the current actor
    // Of course, it's not useful here at the end of the main function, but that's for the example (and to check that this still works in the automated tests)
  }
}
