/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class MigrationManager extends Actor {
  VirtualMachine vm;
  Host dst_pm;
  MigrationManager(VirtualMachine vm, Host dst_pm)
  {
    this.vm     = vm;
    this.dst_pm = dst_pm;
  }
  public void run()
  {
    Host src_pm      = vm.get_pm();
    double mig_start = Engine.get_clock();
    vm.migrate(dst_pm);
    double mig_end = Engine.get_clock();

    Engine.info("%s migrated: %s->%s in %g s", vm.get_name(), src_pm.get_name(), dst_pm.get_name(),
                mig_end - mig_start);
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
    e.add_actor("MigMgr", Host.current(), new MigrationManager(vm0, pm1)).join();

    vm0.destroy();

    vm0 = pm0.create_vm("VM0", 1);
    vm0.set_ramsize(1e8); // 100Mbytes
    vm0.start();

    Engine.info("Test: Migrate a VM with %d Mbytes RAM", vm0.get_ramsize() / 1000 / 1000);
    e.add_actor("MigMgr", Host.current(), new MigrationManager(vm0, pm1)).join();
    vm0.destroy();

    vm0     = pm0.create_vm("VM0", 1);
    VirtualMachine vm1 = pm0.create_vm("VM1", 1);

    vm0.set_ramsize(1e9); // 1Gbytes
    vm1.set_ramsize(1e9); // 1Gbytes
    vm0.start();
    vm1.start();

    Engine.info("Test: Migrate two VMs at once from PM0 to PM1");
    e.add_actor("MigMgr", Host.current(), new MigrationManager(vm0, pm1));
    e.add_actor("MigMgr", Host.current(), new MigrationManager(vm1, pm1));
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
    e.add_actor("MigMgr", Host.current(), new MigrationManager(vm0, pm1));
    e.add_actor("MigMgr", Host.current(), new MigrationManager(vm1, pm2));
    this.sleep_for(10000);

    vm0.destroy();
    vm1.destroy();
  }
}

public class cloud_migration {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);
    e.add_actor("Main", e.host_by_name("Fafard"), new Main());
    e.run();

    Engine.info("Simulation ends.");
  }
}
