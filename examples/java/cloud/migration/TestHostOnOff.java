/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.migration;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostFailureException;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.VM;

// This test aims at validating that the migration process is robust in face of host turning off either on the SRC 
// node or on the DST node. 
public class TestHostOnOff extends Process{

  protected Host host0 = null;
  protected Host host1 = null;
  protected Host host2 = null;

  TestHostOnOff(String hostname, String name) throws  HostNotFoundException {
    super(hostname, name);
  }

  public void main(String[] strings) throws MsgException {
    /* get hosts 1 and 2*/
    try {
      host0 = Host.getByName("PM0");
      host1 = Host.getByName("PM1");
      host2 = Host.getByName("PM2");
    }catch (HostNotFoundException e) {
      Msg.error("You are trying to use a non existing node:" + e.getMessage());
    }

    // Robustness on the SRC node
    for (int i =100 ; i < 55000 ; i+=100)
     testVMMigrate(host1, i);

    // Robustness on the DST node
    for (int i =0 ; i < 55000 ; i++)
      testVMMigrate(host2, i);

    /* End of Tests */
    Msg.info("Nor more tests, Bye Bye !");
    Main.setEndOfTest();
  }

  public void testVMMigrate (Host hostToKill, long killAt) throws MsgException {
    Msg.info("**** **** **** ***** ***** Test Migrate with host shutdown ***** ***** **** **** ****");
    Msg.info("Turn on one host, assign a VM on this host, launch a process inside the VM, migrate the VM and "
             + "turn off either the SRC or DST");

    host1.off();
    host2.off();
    host1.on();
    host2.on();

    // Create VM0
    int dpRate = 70;
    XVM vm0 = null;
    vm0 = new XVM(host1, "vm0",
        2048, // Ramsize,
        125, // Net bandwidth,
        dpRate // Memory intensity
        );
    vm0.start();
    vm0.setLoad(90);

    String[] args = new String[3];

    args[0] = "vm0";
    args[1] = "PM1";
    args[2] = "PM2";
    new Process(host1, "Migrate-" + killAt, args) {
      public void main(String[] args) {
        Host destHost = null;
        Host sourceHost = null;

        try {
          sourceHost = Host.getByName(args[1]);
          destHost = Host.getByName(args[2]);
        } catch (HostNotFoundException e) {
          Msg.error("You are trying to migrate from/to a non existing node: " + e.getMessage());
          e.printStackTrace();
        }
        if (destHost != null && sourceHost.isOn() && destHost.isOn()) {
          try {
            Msg.info("Migrate vm "+args[0]+" to node "+destHost.getName());
            VM.getVMByName(args[0]).migrate(destHost);
          } catch (HostFailureException e) {
            Msg.error("Something occurs during the migration that cannot validate the operation: " + e.getMessage());
            e.printStackTrace();
          }
        }
      }
    }.start();

    // Wait killAt ms before killing thehost
    Process.sleep(killAt);
    Msg.info("The migration process should be stopped and we should catch an exception");
    hostToKill.off();

    Process.sleep(50000);
    Msg.info("Destroy VMs");
    vm0.shutdown();
    Process.sleep(20000);
  }

  public void testVMShutdownDestroy () throws HostFailureException {
    Msg.info("**** **** **** ***** ***** Test shutdown a VM ***** ***** **** **** ****");
    Msg.info("Turn on host1, assign a VM on host1, launch a process inside the VM, and turn off the vm, " +
        "and check whether you can reallocate the same VM");

    // Create VM0
    int dpRate = 70;
    XVM vm0 = new XVM(host1, "vm0",
        2048, // Ramsize,
        125, // Net bandwidth,
        dpRate // Memory intensity
        );
    Msg.info("Start VM0");
    vm0.start();
    vm0.setLoad(90);

    Process.sleep(5000);

    Msg.info("Shutdown VM0");
    vm0.shutdown();
    Process.sleep(5000);

    Msg.info("Restart VM0");
    vm0 = new XVM(host1, "vm0",
        2048, // Ramsize,
        125, // Net bandwidth,
        dpRate // Memory intensity
        );
    vm0.start();
    vm0.setLoad(90);

    Msg.info("You suceed to recreate and restart a VM without generating any exception ! Great the Test is ok");

    Process.sleep(5000);
    vm0.shutdown();
  }
}
