/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package surfCpuModel;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;

public class Receiver extends Process {
   public Receiver(Host host, String name, String[]args) {
		super(host,name,args);
   }
   final double commSizeLat = 1;
   final double commSizeBw = 100000000;

   public void main(String[] args) throws MsgException {

      Msg.info("helloo!");

      Task task;
      task = Task.receive(getHost().getName());
      task.execute();

      Msg.info("goodbye!");
    }
}
