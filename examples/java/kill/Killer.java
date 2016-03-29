/* Master of a basic master/slave example in Java */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package kill;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;

import kill.Victim;

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
      System.out.println("Cannot create the victim process!");
    }
    sleep(10000);
    Msg.info("Resume Process");
    poorVictim.resume();
    sleep(1000);
    Msg.info("Kill Process");
    poorVictim.kill();

    Msg.info("Ok, goodbye now.");
  }
}
