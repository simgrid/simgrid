/* Copyright (c) 2006-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.consumption;

import org.simgrid.msg.Msg;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.HostNotFoundException;

public class EnergyConsumer extends Process {
  public EnergyConsumer(String hostname, String name) throws HostNotFoundException {
    super(hostname,name);
  }

  public void main(String[] args) throws MsgException {
     Msg.info("Energetic profile: " + getHost().getProperty("wattage_per_state"));
     Msg.info("Initial peak speed= " + getHost().getSpeed() + " flop/s; Energy dissipated = "
              + getHost().getConsumedEnergy() + " J");

     this.waitFor(10);
     Msg.info("Currently consumed energy after sleeping 10 sec: "+getHost().getConsumedEnergy());
     new Task(null, 1E9, 0).execute();
     Msg.info("Currently consumed energy after executing 1E9 flops: "+getHost().getConsumedEnergy());
  }
}
