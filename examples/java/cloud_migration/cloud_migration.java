/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class MigrationManager extends Actor {
  VirtualMachine vm;
  Host dstPM;
  MigrationManager(VirtualMachine vm, Host dstPM)
  {
    this.vm     = vm;
    this.dstPM  = dstPM;
  }
  public void run()
  {
    Host src_pm      = vm.get_pm();
    double migStart  = Engine.get_clock();
    vm.migrate(dstPM);
    double migEnd = Engine.get_clock();

    Engine.info("%s migrated: %s->%s in %g s", vm.get_name(), src_pm.get_name(), dstPM.get_name(), migEnd - migStart);
  }
}

class Main extends Actor {
  public void run()
  {
    Engine e = this.get_engine();
    Host pm0 = e.host_by_name("Fafard");
    Host pm1 = e.host_by_name("Tremblay");
    Host pm2 = e.host_by_name("Bourassa");

    VirtualMachine vm0 = pm0.create_vm("VM0", 1);
    vm0.set_ramsize(1e9); // 1Gbytes
    vm0.start();

    Engine.info("Test: Migrate a VM with %d Mbytes RAM", vm0.get_ramsize() / 1000 / 1000);
    Host.current().add_actor("MigMgr", new MigrationManager(vm0, pm1)).join();

    vm0.destroy();

    vm0 = pm0.create_vm("VM0", 1);
    vm0.set_ramsize(1e8); // 100Mbytes
    vm0.start();

    Engine.info("Test: Migrate a VM with %d Mbytes RAM", vm0.get_ramsize() / 1000 / 1000);
    Host.current().add_actor("MigMgr", new MigrationManager(vm0, pm1)).join();
    vm0.destroy();

    vm0     = pm0.create_vm("VM0", 1);
    VirtualMachine vm1 = pm0.create_vm("VM1", 1);

    vm0.set_ramsize(1e9); // 1Gbytes
    vm1.set_ramsize(1e9); // 1Gbytes
    vm0.start();
    vm1.start();

    Engine.info("Test: Migrate two VMs at once from PM0 to PM1");
    Host.current().add_actor("MigMgr", new MigrationManager(vm0, pm1));
    Host.current().add_actor("MigMgr", new MigrationManager(vm1, pm1));
    this.sleep_for(10000);

    vm0.destroy();
    vm1.destroy();

    vm0 = pm0.create_vm("VM0", 1);
    vm1 = pm0.create_vm("VM1", 1);

    vm0.set_ramsize(1e9); // 1Gbytes
    vm1.set_ramsize(1e9); // 1Gbytes
    vm0.start();
    vm1.start();

    Engine.info("Test: Migrate two VMs at once to different PMs");
    Host.current().add_actor("MigMgr", new MigrationManager(vm0, pm1));
    Host.current().add_actor("MigMgr", new MigrationManager(vm1, pm2));
    this.sleep_for(10000);

    vm0.destroy();
    vm1.destroy();
  }
}

public class cloud_migration {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.plugin_vm_live_migration_init();

    e.load_platform(args[0]);
    e.host_by_name("Fafard").add_actor("Main", new Main());
    e.run();

    Engine.info("Simulation ends.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
