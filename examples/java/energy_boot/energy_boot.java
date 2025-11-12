/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is an example of how the bootup and shutdown periods can be modeled
 * with SimGrid, taking both the time and overall consumption into account.
 *
 * The main idea is to augment the platform description to declare fake
 * pstate that represent these states. The CPU speed of these state is zero
 * (the CPU delivers 0 flop per second when booting) while the energy
 * consumption is the one measured on average on the modeled machine.
 *
 * When you want to bootup the machine, you set it into the pstate encoding
 * the boot (3 in this example), and leave it so for the right time using a
 * sleep_for(). During that time no other execution can progress since the
 * resource speed is set at 0 flop/s in this fake pstate. Once this is over,
 * the boot is done and we switch back to the regular pstate. Conversely,
 * the fake pstate 4 is used to encode the shutdown delay.
 *
 * Some people don't like the idea to add fake pstates for the boot time, and
 * would like SimGrid to provide a "cleaner" model for that. But the "right"
 * model depends on the study you want to conduct. If you want to study the
 * instantaneous consumption of a rebooting data center, the model used here
 * is not enough since it considers only the average consumption over the boot,
 * while the instantaneous consumption changes dramatically. Conversely, a
 * model taking the instantaneous changes into account will be very difficult
 * to instantiate correctly (which values will you use?), so it's not adapted
 * to most studies. At least, fake pstates allow you to do exactly what you
 * need for your very study.
 */

import org.simgrid.s4u.*;

class monitor extends Actor {
  void simulate_bootup(Host host)
  {
    int previousPstate = host.get_pstate();

    Engine.info("Switch to virtual pstate 3, that encodes the 'booting up' state in that platform");
    host.set_pstate(3);

    Engine.info("Actually start the host");
    host.turn_on();

    Engine.info("Wait 150s to simulate the boot time.");
    sleep_for(150);

    Engine.info("The host is now up and running. Switch back to previous pstate %d", previousPstate);
    host.set_pstate(previousPstate);
  }

  void simulate_shutdown(Host host)
  {
    int previousPstate = host.get_pstate();

    Engine.info("Switch to virtual pstate 4, that encodes the 'shutting down' state in that platform");
    host.set_pstate(4);

    Engine.info("Wait 7 seconds to simulate the shutdown time.");
    sleep_for(7);

    Engine.info("Switch back to previous pstate %d, that will be used on reboot.", previousPstate);
    host.set_pstate(previousPstate);

    Engine.info("Actually shutdown the host");
    host.turn_off();
  }

  public void run()
  {
    Host host1 = get_engine().host_by_name("MyHost1");

    Engine.info("Initial pstate: %d; Energy dissipated so far:%.0E J", host1.get_pstate(), host1.get_consumed_energy());

    Engine.info("Sleep for 10 seconds");
    sleep_for(10);
    Engine.info("Done sleeping. Current pstate: %d; Energy dissipated so far: %.2f J", host1.get_pstate(),
                host1.get_consumed_energy());

    simulate_shutdown(host1);
    Engine.info("Host1 is now OFF. Current pstate: %d; Energy dissipated so far: %.2f J", host1.get_pstate(),
                host1.get_consumed_energy());

    Engine.info("Sleep for 10 seconds");
    sleep_for(10);
    Engine.info("Done sleeping. Current pstate: %d; Energy dissipated so far: %.2f J", host1.get_pstate(),
                host1.get_consumed_energy());

    simulate_bootup(host1);
    Engine.info("Host1 is now ON again. Current pstate: %d; Energy dissipated so far: %.2f J", host1.get_pstate(),
                host1.get_consumed_energy());
  }
}

public class energy_boot {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);
    e.plugin_host_energy_init();

    if (args.length < 1)
      Engine.die("Usage: java -cp energy_boot.jar energy_boot platform_file\n\tExample: java -cp energy_boot.jar "
                 + "energy_boot platform.xml\n");

    e.load_platform(args[0]);
    e.host_by_name("MyHost2").add_actor("Boot Monitor", new monitor());

    e.run();

    Engine.info("End of simulation.");

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
