/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.TimeoutException;

public class EnergyConsumer extends Process {
  public EnergyConsumer(Host host, String name, String[] args) {
    super(host,name,args);
  }

  @Override
  public void main(String[] args) throws MsgException {
     Msg.info("Currently consumed energy: "+getHost().getConsumedEnergy());
     this.waitFor(10);
     Msg.info("Currently consumed energy after sleeping 10 sec: "+getHost().getConsumedEnergy());
     new Task(null, 1E9, 0).execute();
     Msg.info("Currently consumed energy after executing 1E9 flops: "+getHost().getConsumedEnergy());
  }
}
