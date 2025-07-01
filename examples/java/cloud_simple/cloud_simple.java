/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class ComputationWorker extends Actor {
  public void run()
  {
    double clock_sta = Engine.get_clock();
    this.execute(1000000);
    double clock_end = Engine.get_clock();

    Engine.info("%s:%s executed %g", this.get_host().get_name(), this.get_name(), clock_end - clock_sta);
  }
}

class Payload {
  public Host tx_host;
  public String tx_actor_name;
  public double clock_sta;
}

class CommunicationTX extends Actor {
  String mbox_name;
  CommunicationTX(String mbox_name) { this.mbox_name = mbox_name; }
  public void run()
  {
    Mailbox mbox          = this.get_engine().mailbox_by_name(mbox_name);
    Payload payload       = new Payload();
    payload.tx_actor_name = Actor.self().get_name();
    payload.tx_host       = this.get_host();
    payload.clock_sta     = Engine.get_clock();

    mbox.put(payload, 1000000);
  }
}

class CommunicationRX extends Actor {
  String mbox_name;
  CommunicationRX(String mbox_name) { this.mbox_name = mbox_name; }
  public void run() throws SimgridException
  {
    String actor_name = Actor.self().get_name();
    String host_name  = this.get_host().get_name();
    Mailbox mbox      = this.get_engine().mailbox_by_name(mbox_name);

    Payload payload  = (Payload)mbox.get();
    double clock_end = Engine.get_clock();

    Engine.info("%s:%s to %s:%s => %g sec", payload.tx_host.get_name(), payload.tx_actor_name, host_name, actor_name,
                clock_end - payload.clock_sta);
  }
}

class Main extends Actor {
  void launch_communication_worker(Host tx_host, Host rx_host)
  {
    String mbox_name = "MBOX:" + tx_host.get_name() + "-" + rx_host.get_name();

    tx_host.add_actor("comm_tx", new CommunicationTX(mbox_name));
    rx_host.add_actor("comm_rx", new CommunicationRX(mbox_name));
  }
  public void run()
  {
    Engine e = this.get_engine();
    Host pm0 = e.host_by_name("Fafard");
    Host pm1 = e.host_by_name("Tremblay");
    Host pm2 = e.host_by_name("Bourassa");

    Engine.info("## Test 1 (started): check computation on normal PMs");

    Engine.info("### Put an activity on a PM");
    pm0.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);

    Engine.info("### Put two activities on a PM");
    pm0.add_actor("compute", new ComputationWorker());
    pm0.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);

    Engine.info("### Put an activity on each PM");
    pm0.add_actor("compute", new ComputationWorker());
    pm1.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);

    Engine.info("## Test 1 (ended)");

    Engine.info("## Test 2 (started): check impact of running an activity inside a VM (there is no degradation for "
                + "the moment)");

    Engine.info("### Put a VM on a PM, and put an activity to the VM");
    VirtualMachine vm0 = pm0.create_vm("VM0", 1);
    vm0.start();
    vm0.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);
    vm0.destroy();

    Engine.info("## Test 2 (ended)");

    Engine.info(
        "## Test 3 (started): check impact of running an activity collocated with a VM (there is no VM noise for "
        + "the moment)");

    Engine.info("### Put a VM on a PM, and put an activity to the PM");
    vm0 = pm0.create_vm("VM0", 1);
    vm0.start();
    pm0.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);
    vm0.destroy();
    Engine.info("## Test 3 (ended)");

    Engine.info(
        "## Test 4 (started): compare the cost of running two activities inside two different VMs collocated or not "
        +
        "(for the moment, there is no degradation for the VMs. Hence, the time should be equals to the time of test 1");

    Engine.info("### Put two VMs on a PM, and put an activity to each VM");
    vm0 = pm0.create_vm("VM0", 1);
    vm0.start();
    VirtualMachine vm1 = pm0.create_vm("VM1", 1);
    vm0.add_actor("compute", new ComputationWorker());
    vm1.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);
    vm0.destroy();
    vm1.destroy();

    Engine.info("### Put a VM on each PM, and put an activity to each VM");
    vm0 = pm0.create_vm("VM0", 1);
    vm1 = pm1.create_vm("VM1", 1);
    vm0.start();
    vm1.start();
    vm0.add_actor("compute", new ComputationWorker());
    vm1.add_actor("compute", new ComputationWorker());
    this.sleep_for(2);
    vm0.destroy();
    vm1.destroy();
    Engine.info("## Test 4 (ended)");

    Engine.info("## Test 5  (started): Analyse network impact");
    Engine.info("### Make a connection between PM0 and PM1");
    launch_communication_worker(pm0, pm1);
    this.sleep_for(5);

    Engine.info("### Make two connection between PM0 and PM1");
    launch_communication_worker(pm0, pm1);
    launch_communication_worker(pm0, pm1);
    this.sleep_for(5);

    Engine.info("### Make a connection between PM0 and VM0@PM0");
    vm0 = pm0.create_vm("VM0", 1);
    vm0.start();
    launch_communication_worker(pm0, vm0);
    this.sleep_for(5);
    vm0.destroy();

    Engine.info("### Make a connection between PM0 and VM0@PM1");
    vm0 = pm1.create_vm("VM0", 1);
    launch_communication_worker(pm0, vm0);
    this.sleep_for(5);
    vm0.destroy();

    Engine.info("### Make two connections between PM0 and VM0@PM1");
    vm0 = pm1.create_vm("VM0", 1);
    vm0.start();
    launch_communication_worker(pm0, vm0);
    launch_communication_worker(pm0, vm0);
    this.sleep_for(5);
    vm0.destroy();

    Engine.info("### Make a connection between PM0 and VM0@PM1, and also make a connection between PM0 and PM1");
    vm0 = pm1.create_vm("VM0", 1);
    vm0.start();
    launch_communication_worker(pm0, vm0);
    launch_communication_worker(pm0, pm1);
    this.sleep_for(5);
    vm0.destroy();

    Engine.info(
        "### Make a connection between VM0@PM0 and PM1@PM1, and also make a connection between VM0@PM0 and VM1@PM1");
    vm0 = pm0.create_vm("VM0", 1);
    vm1 = pm1.create_vm("VM1", 1);
    vm0.start();
    vm1.start();
    launch_communication_worker(vm0, vm1);
    launch_communication_worker(vm0, vm1);
    this.sleep_for(5);
    vm0.destroy();
    vm1.destroy();

    Engine.info("## Test 5 (ended)");
    Engine.info(
        "## Test 6 (started): Check migration impact (not yet implemented neither on the CPU resource nor on the"
        + " network one)");
    Engine.info("### Relocate VM0 between PM0 and PM1");
    vm0 = pm0.create_vm("VM0", 1);
    vm0.set_ramsize(1024 * 1024 * 1024).start(); // 1GiB

    launch_communication_worker(vm0, pm2);
    this.sleep_for(0.01);
    vm0.migrate(pm1);
    this.sleep_for(0.01);
    vm0.migrate(pm0);
    this.sleep_for(5);
    vm0.destroy();
    Engine.info("## Test 6 (ended)");
  }
}
public class cloud_simple {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.plugin_vm_live_migration_init();
    e.load_platform(args[0]);

    e.host_by_name("Fafard").add_actor("main", new Main());

    e.run();

    Engine.info("Simulation ends.");
  }
}
