/* Copyright (c) 2006-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.startkilltime;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Sleeper extends Process {
  public Sleeper(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    Msg.info("Hello! I go to sleep.");
    try {
      waitFor(Integer.parseInt(args[0]));
      Msg.info("Done sleeping");
    } catch (MsgException e) {
      Msg.debug("Wait cancelled.");
    }
  }
}
