/* Copyright (c) 2006-2007, 2010, 2013-2014, 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package async;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.TaskCancelledException;
import org.simgrid.msg.TimeoutException;
import org.simgrid.msg.TransferFailureException;

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
    Comm comm = null;
    boolean slaveFinished = false;
    while(!slaveFinished) {  
      try {
        if (comm == null) {
          Msg.info("Receiving on 'slave_" + num + "'");
          comm = Task.irecv("slave_" + num);
        } else {
          if (comm.test()) {
            Task task = comm.getTask();

            if (task instanceof FinalizeTask) {
              comm = null;
              break;
            }
            Msg.info("Received a task");
            Msg.info("Received \"" + task.getName() +  "\". Processing it.");
            try {
              task.execute();
            } catch (TaskCancelledException e) {
            
            }
            comm = null;
          } else {
            waitFor(1);
          }
        }
      }
      catch (Exception e) {
        e.printStackTrace();
      }
    }
    Msg.info("Received Finalize. I'm done. See you!");
    waitFor(20);
  }
}