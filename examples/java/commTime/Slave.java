/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package commTime;
import org.simgrid.msg.*;

public class Slave extends org.simgrid.msg.Process {
  public Slave(Host host, String name, String[]args) {
    super(host,name,args);
  }
  public void main(String[] args) throws MsgException {
    if (args.length < 1) {
       Msg.info("Slave needs 1 argument (its number)");
       System.exit(1);
    }

    int num = Integer.valueOf(args[0]).intValue();
    Msg.info("Receiving on 'slave_"+num+"'");

    while(true) { 
      Task task = Task.receive("slave_"+num);  
      if (task instanceof FinalizeTask) {
        break;
      }
      task.execute();
    }
    Msg.info("Received Finalize. I'm done. See you!");
  }
}