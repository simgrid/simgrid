/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package masterslave;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Task;
import org.simgrid.msg.TaskCancelledException;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;
import org.simgrid.msg.Process;

public class Slave extends Process {
  public Slave(Host host, String name, String[]args) {
    super(host,name,args);
  }
  public void main(String[] args) throws TransferFailureException, HostFailureException, TimeoutException {
    if (args.length < 1) {
      Msg.info("Slave needs 1 argument (its number)");
      System.exit(1);
    }

    int num = Integer.valueOf(args[0]).intValue();
    //Msg.info("Receiving on 'slave_"+num+"'");

    while(true) {  
      Task task = Task.receive("slave_"+num);  

      if (task instanceof FinalizeTask) {
        break;
      }
      Msg.info("Received \"" + task.getName() +  "\". Processing it.");
      try {
        task.execute();
      } catch (TaskCancelledException e) {}
    }

    Msg.info("Received Finalize. I'm done. See you!");
  }
}
