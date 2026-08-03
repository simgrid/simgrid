/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class Worker extends Actor {
  double computation_amount;
  boolean use_bound;
  double bound;

  public Worker(double computation_amount, boolean use_bound, double bound)
  {
    this.computation_amount = computation_amount;
    this.use_bound          = use_bound;
    this.bound              = bound;
  }

  public void run() throws SimgridException
  {
    double clock_start = Engine.get_clock();

    Exec exec = this.exec_init(computation_amount);

    if (use_bound) {
      if (bound < 1e-12) // close enough to 0 without any floating precision surprise
        Engine.info("bound == 0 means no capping (i.e., unlimited).");
      exec.set_bound(bound);
    }
    exec.start();
    exec.await();
    double clock_end     = Engine.get_clock();
    double duration      = clock_end - clock_start;
    double flops_per_sec = computation_amount / duration;

    if (use_bound)
      Engine.info("bound to %f => duration %f (%f flops/s)", bound, duration, flops_per_sec);
    else
      Engine.info("not bound => duration %f (%f flops/s)", duration, flops_per_sec);
  }
}

class WorkerBusyLoop extends Actor {
  String name;
  double speed;

  public WorkerBusyLoop(String name, double speed)
  {
    this.name  = name;
    this.speed = speed;
  }

  public void run() throws SimgridException
  {
    double exec_remain_prev = 1e11;
    Exec exec               = this.exec_async(exec_remain_prev);
    for (int i = 0; i < 10; i++) {
      if (speed > 0) {
        double new_bound = (speed / 10) * i;
        Engine.info("set bound of VM1 to %f", new_bound);
        if (!(this.get_host() instanceof VirtualMachine))
          Engine.critical("The host of %s is not a VM. Its name is %s", get_name(), get_host().get_name());
        ((VirtualMachine)this.get_host()).set_bound(new_bound);
      }
      this.sleep_for(100);
      double exec_remain_now = exec.get_remaining();
      double flops_per_sec   = exec_remain_prev - exec_remain_now;
      Engine.info("%s@%s: %.0f flops/s", name, this.get_host().get_name(), flops_per_sec / 100);
      exec_remain_prev = exec_remain_now;
      this.sleep_for(1);
    }
    exec.await();
  }
}

class Master extends Actor {
  public void run() throws SimgridException { master_main(this); }

  static void test_dynamic_change(Master self) throws SimgridException
  {
    Host pm0 = self.get_engine().host_by_name("Fafard");

    VirtualMachine vm0 = pm0.create_vm("VM0", 1);
    VirtualMachine vm1 = pm0.create_vm("VM1", 1);
    vm0.start();
    vm1.start();

    vm0.add_actor("worker0", new WorkerBusyLoop("Task0", -1));
    vm1.add_actor("worker1", new WorkerBusyLoop("Task1", pm0.get_speed()));

    self.sleep_for(3000); // let the activities end
    vm0.destroy();
    vm1.destroy();
  }

  static void test_one_activity(Master self, Host host) throws SimgridException
  {
    double cpu_speed          = host.get_speed();
    double computation_amount = cpu_speed * 10;

    Engine.info("### Test: with/without activity set_bound");

    Engine.info("### Test: no bound for Task1@%s", host.get_name());
    host.add_actor("worker0", new Worker(computation_amount, false, 0));

    self.sleep_for(1000);

    Engine.info("### Test: 50%% for Task1@%s", host.get_name());
    host.add_actor("worker0", new Worker(computation_amount, true, cpu_speed / 2));

    self.sleep_for(1000);

    Engine.info("### Test: 33%% for Task1@%s", host.get_name());
    host.add_actor("worker0", new Worker(computation_amount, true, cpu_speed / 3));

    self.sleep_for(1000);

    Engine.info("### Test: zero for Task1@%s (i.e., unlimited)", host.get_name());
    host.add_actor("worker0", new Worker(computation_amount, true, 0));

    self.sleep_for(1000);

    Engine.info("### Test: 200%% for Task1@%s (i.e., meaningless)", host.get_name());
    host.add_actor("worker0", new Worker(computation_amount, true, cpu_speed * 2));

    self.sleep_for(1000);
  }

  static void test_two_activities(Master self, Host hostA, Host hostB) throws SimgridException
  {
    double cpu_speed = hostA.get_speed();
    if (cpu_speed != hostB.get_speed())
      Engine.die("Both hosts are supposed to have the same speed");
    double computation_amount = cpu_speed * 10;
    String hostA_name         = hostA.get_name();
    String hostB_name         = hostB.get_name();

    Engine.info("### Test: no bound for Task1@%s, no bound for Task2@%s", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, false, 0));
    hostB.add_actor("worker1", new Worker(computation_amount, false, 0));

    self.sleep_for(1000);

    Engine.info("### Test: 0 for Task1@%s, 0 for Task2@%s (i.e., unlimited)", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, true, 0));
    hostB.add_actor("worker1", new Worker(computation_amount, true, 0));

    self.sleep_for(1000);

    Engine.info("### Test: 50%% for Task1@%s, 50%% for Task2@%s", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, true, cpu_speed / 2));
    hostB.add_actor("worker1", new Worker(computation_amount, true, cpu_speed / 2));

    self.sleep_for(1000);

    Engine.info("### Test: 25%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, true, cpu_speed / 4));
    hostB.add_actor("worker1", new Worker(computation_amount, true, cpu_speed / 4));

    self.sleep_for(1000);

    Engine.info("### Test: 75%% for Task1@%s, 100%% for Task2@%s", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, true, cpu_speed * 0.75));
    hostB.add_actor("worker1", new Worker(computation_amount, true, cpu_speed));

    self.sleep_for(1000);

    Engine.info("### Test: no bound for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, false, 0));
    hostB.add_actor("worker1", new Worker(computation_amount, true, cpu_speed / 4));

    self.sleep_for(1000);

    Engine.info("### Test: 75%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
    hostA.add_actor("worker0", new Worker(computation_amount, true, cpu_speed * 0.75));
    hostB.add_actor("worker1", new Worker(computation_amount, true, cpu_speed / 4));

    self.sleep_for(1000);
  }

  static void master_main(Master self) throws SimgridException
  {
    Host pm0 = self.get_engine().host_by_name("Fafard");

    Engine.info("# 1. Put a single activity on a PM.");
    test_one_activity(self, pm0);
    Engine.info(".");

    Engine.info("# 2. Put two activities on a PM.");
    test_two_activities(self, pm0, pm0);
    Engine.info(".");

    VirtualMachine vm0 = pm0.create_vm("VM0", 1);
    vm0.start();

    Engine.info("# 3. Put a single activity on a VM.");
    test_one_activity(self, vm0);
    Engine.info(".");

    Engine.info("# 4. Put two activities on a VM.");
    test_two_activities(self, vm0, vm0);
    Engine.info(".");

    vm0.destroy();

    vm0 = pm0.create_vm("VM0", 1);
    vm0.start();

    Engine.info("# 6. Put an activity on a PM and an activity on a VM.");
    test_two_activities(self, pm0, vm0);
    Engine.info(".");

    vm0.destroy();

    vm0 = pm0.create_vm("VM0", 1);
    vm0.set_bound(pm0.get_speed() / 10);
    vm0.start();

    Engine.info("# 7. Put a single activity on the VM capped by 10%%.");
    test_one_activity(self, vm0);
    Engine.info(".");

    Engine.info("# 8. Put two activities on the VM capped by 10%%.");
    test_two_activities(self, vm0, vm0);
    Engine.info(".");

    Engine.info("# 9. Put an activity on a PM and an activity on the VM capped by 10%%.");
    test_two_activities(self, pm0, vm0);
    Engine.info(".");

    vm0.destroy();

    vm0 = pm0.create_vm("VM0", 1);
    vm0.set_ramsize(1e9); // 1GB
    vm0.start();

    double cpu_speed = pm0.get_speed();

    Engine.info("# 10. Test migration");
    double computation_amount = cpu_speed * 10;

    Engine.info("# 10. (a) Put an activity on a VM without any bound.");
    vm0.add_actor("worker0", new Worker(computation_amount, false, 0));
    self.sleep_for(1000);
    Engine.info(".");

    Engine.info("# 10. (b) set 10%% bound to the VM, and then put an activity on the VM.");
    vm0.set_bound(cpu_speed / 10);
    vm0.add_actor("worker0", new Worker(computation_amount, false, 0));
    self.sleep_for(1000);
    Engine.info(".");

    Engine.info("# 10. (c) migrate");
    Host pm1 = self.get_engine().host_by_name("Fafard");
    vm0.migrate(pm1);
    Engine.info(".");

    Engine.info("# 10. (d) Put an activity again on the VM.");
    vm0.add_actor("worker0", new Worker(computation_amount, false, 0));
    self.sleep_for(1000);
    Engine.info(".");

    vm0.destroy();

    Engine.info("# 11. Change a bound dynamically.");
    test_dynamic_change(self);
  }
}

public class cloud_capping {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    args     = e.get_args(); // Remove the logging extra parameters

    e.plugin_vm_live_migration_init();
    /* load the platform file */
    if (args.length != 1)
      Engine.die("Usage: cloud_capping platform_file\n\tExample: small_platform.xml\n");

    e.load_platform(args[0]);

    e.host_by_name("Fafard").add_actor("master_", new Master());

    e.run();
    Engine.info("Bye (simulation time %g)", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
