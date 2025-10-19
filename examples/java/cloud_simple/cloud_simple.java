/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class ComputationWorker extends Actor {
  public void run()
  {
    double clockStart = Engine.get_clock();
    this.execute(1000000);
    double clockEnd = Engine.get_clock();

    Engine.info("%s:%s executed %g", this.get_host().get_name(), this.get_name(), clockEnd - clockStart);
  }
}

class Payload {
  public final Host txHost;
  public final String txActorName;
  public final double clockStart;
  Payload(Host h, String n)
  {
    txHost      = h;
    txActorName = n;
    clockStart  = Engine.get_clock();
  }
}

class CommunicationTX extends Actor {
  String mboxName;
  CommunicationTX(String mboxName) { this.mboxName = mboxName; }
  public void run()
  {
    Mailbox mbox          = this.get_engine().mailbox_by_name(mboxName);
    Payload payload       = new Payload(this.get_host(), Actor.self().get_name());

    mbox.put(payload, 1000000);
  }
}

class CommunicationRX extends Actor {
  String mboxName;
  CommunicationRX(String mboxName) { this.mboxName = mboxName; }
  public void run() throws SimgridException
  {
    String actor_name = Actor.self().get_name();
    String host_name  = this.get_host().get_name();
    Mailbox mbox      = this.get_engine().mailbox_by_name(mboxName);

    Payload payload  = (Payload)mbox.get();
    double clock_end = Engine.get_clock();

    Engine.info("%s:%s to %s:%s => %g sec", payload.txHost.get_name(), payload.txActorName, host_name, actor_name,
                clock_end - payload.clockStart);
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

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
