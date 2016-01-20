/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//package cloud.energy;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

public class Test extends Process{


    public class DummyProcess extends Process {
	        public  DummyProcess (Host host, String name) {
               super(host, name); 
           }

           public void main(String[] args) {		
              Task  task = new Task(this.getHost().getName()+"-task", 30E6 , 0);
              try {
                  task.execute();   
              } catch (Exception e) {
                  e.printStackTrace();
              } 
              //task.finalize();
              Msg.info("This worker is done."); 
           }
        }      
 
    Test(Host host, String name, String[] args) throws HostNotFoundException, NativeException  {
        super(host, name, args);
    }

    public void main(String[] strings) throws MsgException {

       double startTime = 0;
       double endTime = 0;

       /* get hosts */
        Host host1 = null;
        Host host2 = null;
        Host host3 = null;

        try {
            host1 = Host.getByName("MyHost1");
            host2 = Host.getByName("MyHost2");
            host3 = Host.getByName("MyHost3");
        } catch (HostNotFoundException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }


      /* Host 1*/
		  Msg.info("Creating and starting two VMs");
        VM vmHost1 = null;
        vmHost1 = new VM(
                host1,
                "vmHost1",
                4, // Nb of vcpu
                2048, // Ramsize,
                100, // Net Bandwidth
                null, //VM disk image
                1024 * 20,   //size of disk image,
                10, // Net bandwidth,
                50 // Memory intensity
        );
        vmHost1.start();

		  VM vmHost3 = null;
        vmHost3 = new VM(
                host3,
                "vmHost3",
                4, // Nb of vcpu
                2048, // Ramsize,
                100, // Net Bandwidth
                null, //VM disk image
                1024 * 20,   //size of disk image,
                10, // Net bandwidth,
                50 // Memory intensity
        );
        vmHost3.start();

        Msg.info("Create two tasks on Host1: one inside a VM, the other directly on the host");
		  DummyProcess p11 = new DummyProcess (vmHost1, "p11"); 
        p11.run(); 
		  DummyProcess p12 = new DummyProcess (host1, "p12"); 
        p12.run(); 
   
        Msg.info("Create two tasks on Host2: both directly on the host");
		  DummyProcess p21 = new DummyProcess (host2, "p21"); 
        p21.run(); 
		  DummyProcess p22 = new DummyProcess (host2, "p22"); 
        p22.run(); 
 
        Msg.info("Create two tasks on Host3: both inside a VM");
		  DummyProcess p31 = new DummyProcess (vmHost3, "p31"); 
        p31.run(); 
		  DummyProcess p32 = new DummyProcess (vmHost3, "p312"); 
        p32.run(); 
  
        Msg.info("Wait 5 seconde. The tasks are still runing (they run for 3 secondes, but 2 tasks are co-located, so they run for 6 seconds)"); 
        Process.sleep(5); 
        Msg.info("Wait another 5 seconds. The tasks stop at some point in between"); 
        Process.sleep(5); 

        vmHost1.shutdown(); 
        vmHost3.shutdown(); 
     }
}
