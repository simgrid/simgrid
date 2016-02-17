/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.energy;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

/* This class is a process in charge of running the test. It creates and starts the VMs, and fork processes within VMs */
public class EnergyVMRunner extends Process {

  public class DummyProcess extends Process {
    public  DummyProcess (Host host, String name) {
      super(host, name); 
    }

    public void main(String[] args) {    
      Task  task = new Task(this.getHost().getName()+"-task", 300E6 , 0);
      try {
        task.execute();   
      } catch (Exception e) {
        e.printStackTrace();
      } 
      Msg.info("This worker is done."); 
    }
  }

  EnergyVMRunner(Host host, String name, String[] args) throws HostNotFoundException, NativeException  {
    super(host, name, args);
  }

  public void main(String[] strings) throws MsgException, HostNotFoundException {
    double startTime = 0;
    double endTime = 0;

    /* get hosts */
    Host host1 = Host.getByName("MyHost1");
    Host host2 = Host.getByName("MyHost2");
    Host host3 = Host.getByName("MyHost3");

    Msg.info("Creating and starting two VMs");
    VM vmHost1 = new VM(host1, "vmHost1", 4, 2048, 100, null, 1024 * 20, 10,50);
    vmHost1.start();

    VM vmHost3 = new VM(host3, "vmHost3", 4, 2048, 100, null, 1024 * 20, 10,50);
    vmHost3.start();

    Msg.info("Create two tasks on Host1: one inside a VM, the other directly on the host");
    new DummyProcess (vmHost1, "p11"); 
    new DummyProcess (host1, "p12"); 

    Msg.info("Create two tasks on Host2: both directly on the host");
    new DummyProcess (host2, "p21"); 
    new DummyProcess (host2, "p22"); 

    Msg.info("Create two tasks on Host3: both inside a VM");
    new DummyProcess (vmHost3, "p31"); 
    new DummyProcess (vmHost3, "p312"); 

    Msg.info("Wait 5 seconds. The tasks are still running (they run for 3 seconds, but 2 tasks are co-located, "
             + "so they run for 6 seconds)"); 
    waitFor(5); 
    Msg.info("Wait another 5 seconds. The tasks stop at some point in between"); 
    waitFor(5); 

    vmHost1.shutdown(); 
    vmHost3.shutdown(); 
  }
}
