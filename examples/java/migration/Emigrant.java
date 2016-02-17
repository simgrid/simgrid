/* Copyright (c) 2012-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package migration;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Emigrant extends Process {
  public Emigrant(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {    
    Migration.mutex.acquire();
    
    Msg.info("I'll look for a new job on another machine where the grass is greener.");
    migrate(Host.getByName("Boivin"));

    Msg.info("Yeah, found something to do");
    Task task = new Task("job", 98095000, 0);
    task.execute();
    waitFor(2);

    Msg.info("Moving back to home after work");
    migrate(Host.getByName("Jacquelin"));
    migrate(Host.getByName("Boivin"));
    waitFor(4);

    Migration.processToMigrate = this;
    Migration.mutex.release();
    suspend();

    Msg.info("I've been moved on this new host:" + getHost().getName());
    Msg.info("Uh, nothing to do here. Stopping now");
  }
}