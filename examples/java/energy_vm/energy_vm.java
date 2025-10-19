/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Executor extends Actor {
  public void run()
  {
    execute(300E6);
    Engine.info("This worker is done.");
  }
}

class dvfs extends Actor {
  public void run()
  {
    Engine e = get_engine();

    Host host1 = e.host_by_name("MyHost1");
    Host host2 = e.host_by_name("MyHost2");
    Host host3 = e.host_by_name("MyHost3");

    /* Host 1 */
    Engine.info("Creating and starting two VMs");
    var vm_host1 = host1.create_vm("vm_host1", 1);
    vm_host1.start();
    var vm_host2 = host2.create_vm("vm_host2", 1);
    vm_host2.start();

    Engine.info("Create two activities on Host1: both inside a VM");
    vm_host1.add_actor("p11", new Executor());
    vm_host1.add_actor("p12", new Executor());

    Engine.info("Create two activities on Host2: one inside a VM, the other directly on the host");
    vm_host2.add_actor("p21", new Executor());
    host2.add_actor("p22", new Executor());

    Engine.info("Create two activities on Host3: both directly on the host");
    host3.add_actor("p31", new Executor());
    host3.add_actor("p32", new Executor());

    Engine.info("Wait 5 seconds. The activities are still running (they run for 3 seconds, but 2 activities are "
                + "co-located, so they run for 6 seconds)");
    sleep_for(5);
    Engine.info("Wait another 5 seconds. The activities stop at some point in between");
    sleep_for(5);

    vm_host1.destroy();
    vm_host2.destroy();
  }
}

public class energy_vm {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.plugin_host_energy_init();

    if (args.length < 1)
      Engine.die("Usage: java -cp energy_vm.jar energy_vm platform_file\n\tExample: java -cp energy_vm.jar energy_vm "
                 + "../platforms/energy_platform.xml\n");

    e.load_platform(args[0]);
    e.host_by_name("MyHost1").add_actor("dvfs", new dvfs());
    e.run();

    Engine.info("Simulation ends. Host2 and Host3 must have the exact same energy consumption; Host1 "
                    + "is multi-core and will differ.",
                Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
