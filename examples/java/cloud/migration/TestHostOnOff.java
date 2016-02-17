/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.migration;

import java.util.ArrayList;
import java.util.List;
import java.util.Random; 

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

// This test aims at validating that the migration process is robust in face of host turning off either on the SRC 
// node or on the DST node. 
public class TestHostOnOff extends Process{    

  public static Host host0 = null;
  public static Host host1 = null;
  public static Host host2 = null;


  TestHostOnOff(Host host, String name, String[] args) throws HostNotFoundException, NativeException {
    super(host, name, args);
  }

  public void main(String[] strings) throws MsgException {
    double startTime = 0;
    double endTime = 0;

    /* get hosts 1 and 2*/
    try {
      host0 = Host.getByName("host0");
      host1 = Host.getByName("host1");
      host1 = Host.getByName("host2");
    }catch (HostNotFoundException e) {
      e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
    }

    // Robustness on the SRC node
    //for (int i =0 ; i < 55000 ; i++)
    //  test_vm_migrate(host1, i);

    // Robustness on the DST node
    //for (int i =0 ; i < 55000 ; i++)
    //  test_vm_migrate(host2, i);

    /* End of Tests */
    Msg.info("Nor more tests, Bye Bye !");
    Main.setEndOfTest();
  }

  public static void test_vm_migrate (Host hostToKill, long killAt) throws MsgException {
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
        1, // Nb of vcpu
        2048, // Ramsize,
        125, // Net Bandwidth
        null, //VM disk image
        -1,   //size of disk image,
        125, // Net bandwidth,
        dpRate // Memory intensity
        );
    vm0.start();
    vm0.setLoad(90);

    String[] args = new String[3];

    args[0] = "vm0";
    args[1] = "host1";
    args[2] = "host2";
    new Process(host1, "Migrate-" + new Random().nextDouble(), args) {
      public void main(String[] args) {
        Host destHost = null;
        Host sourceHost = null;

        try {
          sourceHost = Host.getByName(args[1]);
          destHost = Host.getByName(args[2]);
        } catch (Exception e) {
          e.printStackTrace();
          System.err.println("You are trying to migrate from/to a non existing node");
        }
        if (destHost != null) {
          if (sourceHost.isOn() && destHost.isOn()) {
            try {
              Msg.info("Migrate vm "+args[0]+" to node "+destHost.getName());
              VM.getVMByName(args[0]).migrate(destHost);
            } catch (HostFailureException e) {
              e.printStackTrace();
              Msg.info("Something occurs during the migration that cannot validate the operation");
            }
          }
        }
      }
    }.start();

    // Wait killAt ms before killing thehost
    Process.sleep(killAt);
    hostToKill.off();
    Process.sleep(5);
    Msg.info("The migration process should be stopped and we should catch an exception\n");
    Process.sleep(5);

    Process.sleep(50000);
    Msg.info("Destroy VMs");
    vm0.shutdown();
    Process.sleep(20000);
  }

  public static void test_vm_shutdown_destroy () throws HostFailureException {
    Msg.info("**** **** **** ***** ***** Test shutdown a VM ***** ***** **** **** ****");
    Msg.info("Turn on host1, assign a VM on host1, launch a process inside the VM, and turn off the vm, " +
        "and check whether you can reallocate the same VM");

    // Create VM0
    int dpRate = 70;
    XVM vm0 = null;
    vm0 = new XVM(host1, "vm0",
        1, // Nb of vcpu
        2048, // Ramsize,
        125, // Net Bandwidth
        null, //VM disk image
        -1,   //size of disk image,
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
        1, // Nb of vcpu
        2048, // Ramsize,
        125, // Net Bandwidth
        null, //VM disk image
        -1,   //size of disk image,
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
