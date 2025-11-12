/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import org.simgrid.s4u.*;

class runner extends Actor {
  public void run() throws HostFailureException
  {
    Host host1   = get_engine().host_by_name("MyHost1");
    Host host2   = get_engine().host_by_name("MyHost2");
    Host[] hosts = new Host[] {host1, host2};

    double old_energy_host1 = host1.get_consumed_energy();
    double old_energy_host2 = host2.get_consumed_energy();

    Engine.info("[%s] Energetic profile: %s", host1.get_name(), host1.get_property("wattage_per_state"));
    Engine.info("[%s] Initial peak speed=%.0E flop/s; Total energy dissipated =%.0E J", host1.get_name(),
                host1.get_speed(), old_energy_host1);
    Engine.info("[%s] Energetic profile: %s", host2.get_name(), host2.get_property("wattage_per_state"));
    Engine.info("[%s] Initial peak speed=%.0E flop/s; Total energy dissipated =%.0E J", host2.get_name(),
                host2.get_speed(), old_energy_host2);

    double start = Engine.get_clock();
    Engine.info("Sleep for 10 seconds");
    sleep_for(10);

    double new_energy_host1 = host1.get_consumed_energy();
    double new_energy_host2 = host2.get_consumed_energy();
    Engine.info("Done sleeping (duration: %.2f s).\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n",
                Engine.get_clock() - start, host1.get_name(), host1.get_speed(), (new_energy_host1 - old_energy_host1),
                host1.get_consumed_energy(), host2.get_name(), host2.get_speed(), (new_energy_host2 - old_energy_host2),
                host2.get_consumed_energy());

    old_energy_host1 = new_energy_host1;
    old_energy_host2 = new_energy_host2;

    // ========= Execute something =========
    start                = Engine.get_clock();
    double flopAmount    = 1E9;
    double[] cpu_amounts = new double[] {flopAmount, flopAmount};
    double[] com_amounts = new double[] {0, 0, 0, 0};
    Engine.info("Run a task of %.0E flops on two hosts", flopAmount);
    parallel_execute(hosts, cpu_amounts, com_amounts);

    new_energy_host1 = host1.get_consumed_energy();
    new_energy_host2 = host2.get_consumed_energy();
    Engine.info("Task done (duration: %.2f s).\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n",
                Engine.get_clock() - start, host1.get_name(), host1.get_speed(), (new_energy_host1 - old_energy_host1),
                host1.get_consumed_energy(), host2.get_name(), host2.get_speed(), (new_energy_host2 - old_energy_host2),
                host2.get_consumed_energy());

    old_energy_host1 = new_energy_host1;
    old_energy_host2 = new_energy_host2;

    // ========= Change power peak =========
    int pstate = 2;
    host1.set_pstate(pstate);
    host2.set_pstate(pstate);
    Engine.info("========= Requesting pstate %d for both hosts (speed should be of %.0E flop/s and is of %.0E flop/s)",
                pstate, host1.get_pstate_speed(pstate), host1.get_speed());

    // ========= Run another ptask =========
    start                 = Engine.get_clock();
    double[] cpu_amounts2 = new double[] {flopAmount, flopAmount};
    double[] com_amounts2 = new double[] {0, 0, 0, 0};
    Engine.info("Run a task of %.0E flops on %s and %.0E flops on %s.", flopAmount, host1.get_name(), flopAmount,
                host2.get_name());
    parallel_execute(hosts, cpu_amounts2, com_amounts2);

    new_energy_host1 = host1.get_consumed_energy();
    new_energy_host2 = host2.get_consumed_energy();
    Engine.info("Task done (duration: %.2f s).\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n",
                Engine.get_clock() - start, host1.get_name(), host1.get_speed(), (new_energy_host1 - old_energy_host1),
                host1.get_consumed_energy(), host2.get_name(), host2.get_speed(), (new_energy_host2 - old_energy_host2),
                host2.get_consumed_energy());

    old_energy_host1 = new_energy_host1;
    old_energy_host2 = new_energy_host2;

    // ========= A new ptask with computation and communication =========
    start                 = Engine.get_clock();
    double comAmount      = 1E7;
    double[] cpu_amounts3 = new double[] {flopAmount, flopAmount};
    double[] com_amounts3 = new double[] {0, comAmount, comAmount, 0};
    Engine.info("Run a task with computation and communication on two hosts.");
    parallel_execute(hosts, cpu_amounts3, com_amounts3);

    new_energy_host1 = host1.get_consumed_energy();
    new_energy_host2 = host2.get_consumed_energy();
    Engine.info("Task done (duration: %.2f s).\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n",
                Engine.get_clock() - start, host1.get_name(), host1.get_speed(), (new_energy_host1 - old_energy_host1),
                host1.get_consumed_energy(), host2.get_name(), host2.get_speed(), (new_energy_host2 - old_energy_host2),
                host2.get_consumed_energy());

    old_energy_host1 = new_energy_host1;
    old_energy_host2 = new_energy_host2;

    // ========= A new ptask with communication only =========
    start                 = Engine.get_clock();
    double[] cpu_amounts4 = new double[] {0, 0};
    double[] com_amounts4 = new double[] {0, comAmount, comAmount, 0};
    Engine.info("Run a task with only communication on two hosts.");
    parallel_execute(hosts, cpu_amounts4, com_amounts4);

    new_energy_host1 = host1.get_consumed_energy();
    new_energy_host2 = host2.get_consumed_energy();
    Engine.info("Task done (duration: %.2f s).\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n",
                Engine.get_clock() - start, host1.get_name(), host1.get_speed(), (new_energy_host1 - old_energy_host1),
                host1.get_consumed_energy(), host2.get_name(), host2.get_speed(), (new_energy_host2 - old_energy_host2),
                host2.get_consumed_energy());

    old_energy_host1 = new_energy_host1;
    old_energy_host2 = new_energy_host2;

    // ========= A new ptask with computation and a timeout =========
    start = Engine.get_clock();
    Engine.info("Run a task with computation on two hosts and a timeout of 20s.");
    try {
      double[] cpu_amounts5 = new double[] {flopAmount, flopAmount};
      double[] com_amounts5 = new double[] {0, 0, 0, 0};
      exec_init(hosts, cpu_amounts5, com_amounts5).await_for(20);
    } catch (TimeoutException ex) {
      Engine.info("Finished WITH timeout");
    }

    new_energy_host1 = host1.get_consumed_energy();
    new_energy_host2 = host2.get_consumed_energy();
    Engine.info("Task ended (duration: %.2f s).\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n"
                    + "[%s] Current peak speed=%.0E flop/s; "
                    + "Energy dissipated during this step=%.2f J; Total energy dissipated=%.0f J\n",
                Engine.get_clock() - start, host1.get_name(), host1.get_speed(), (new_energy_host1 - old_energy_host1),
                host1.get_consumed_energy(), host2.get_name(), host2.get_speed(), (new_energy_host2 - old_energy_host2),
                host2.get_consumed_energy());

    Engine.info("Now is time to quit!");
  }
}

public class energy_exec_ptask {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.plugin_host_energy_init();
    Engine.set_config("host/model:ptask_L07");

    if (args.length < 1)
      Engine.die("Usage: java -cp energy_exec_ptask.jar energy_exec_ptask platform_file\n"
                 + "\tExample: java -cp energy_exec_ptask.jar energy_exec_ptask ../platforms/energy_platform.xml");

    e.load_platform(args[0]);
    e.host_by_name("MyHost1").add_actor("energy_ptask_test", new runner());

    e.run();
    Engine.info("End of simulation.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
