/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package energy.vm;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;
import org.simgrid.msg.TaskCancelledException;
import org.simgrid.msg.VM;

/* This class is a process in charge of running the test. It creates and starts the VMs, and fork processes within VMs */
public class EnergyVMRunner extends Process {

  public class DummyProcess extends Process {
    public  DummyProcess (Host host, String name) {
      super(host, name); 
    }

    @Override
    public void main(String[] strings) {
      Task  task = new Task(this.getHost().getName()+"-task", 300E6 , 0);
      try {
        task.execute();
      } catch (HostFailureException | TaskCancelledException e) {
        Msg.error(e.getMessage());
        e.printStackTrace();
      } 
      Msg.info("This worker is done."); 
    }
  }

  EnergyVMRunner(Host host, String name, String[] args) throws HostNotFoundException {
    super(host, name, args);
  }

  @Override
  public void main(String[] strings) throws HostNotFoundException, HostFailureException {
    /* get hosts */
    Host host1 = Host.getByName("MyHost1");
    Host host2 = Host.getByName("MyHost2");
    Host host3 = Host.getByName("MyHost3");

    Msg.info("Creating and starting two VMs");
    VM vmHost1 = new VM(host1, "vmHost1");
    vmHost1.start();

    VM vmHost2 = new VM(host2, "vmHost3");
    vmHost2.start();

    Msg.info("Create two tasks on Host1: one inside a VM, the other directly on the host");
    new DummyProcess (vmHost1, "p11").start(); 
    new DummyProcess (vmHost1, "p12").start(); 

    Msg.info("Create two tasks on Host2: both directly on the host");
    new DummyProcess (vmHost2, "p21").start();
    new DummyProcess (host2, "p22").start();

    Msg.info("Create two tasks on Host3: both inside a VM");
    new DummyProcess (host3, "p31").start();
    new DummyProcess (host3, "p312").start();

    Msg.info("Wait 5 seconds. The tasks are still running (they run for 3 seconds, but 2 tasks are co-located, "
             + "so they run for 6 seconds)"); 
    waitFor(5);
    Msg.info("Wait another 5 seconds. The tasks stop at some point in between"); 
    waitFor(5);

    vmHost1.destroy();
    vmHost2.destroy();
  }
}
