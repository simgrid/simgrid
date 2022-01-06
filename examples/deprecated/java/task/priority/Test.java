/* Copyright (c) 2012-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package task.priority;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;

public class Test extends Process {
  public Test(Host host, String name, String[]args) {
    super(host,name,args);
  }

  public void main(String[] args) throws MsgException {
    double computationAmount = Double.parseDouble(args[0]);
    double priority = Double.parseDouble(args[1]);

    Msg.info("Hello! Running a task of size " + computationAmount + " with priority " + priority);

    Task task = new Task("Task", computationAmount, 0);
    task.setPriority(priority);

    task.execute();

    Msg.info("Goodbye now!");
  }
}
