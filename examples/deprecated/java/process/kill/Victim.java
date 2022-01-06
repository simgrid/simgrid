/* Copyright (c) 2006-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package process.kill;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;

public class Victim extends Process {
  public Victim(String hostname, String name) throws HostNotFoundException {
    super(hostname, name);
  }
  public void main(String[] args) throws MsgException{
    Msg.info("Hello!");
    Msg.info("Suspending myself");
    suspend();
    Msg.info("OK, OK. Let's work");
    Task task = new Task("work", 1e9, 0);
    task.execute();
    Msg.info("Bye");
  }
}