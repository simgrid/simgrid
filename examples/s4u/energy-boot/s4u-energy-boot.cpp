/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

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

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this example");

static void simulate_bootup(simgrid::s4u::Host* host)
{
  int previous_pstate = host->get_pstate();

  XBT_INFO("Switch to virtual pstate 3, that encodes the 'booting up' state in that platform");
  host->set_pstate(3);

  XBT_INFO("Actually start the host");
  host->turn_on();

  XBT_INFO("Wait 150s to simulate the boot time.");
  simgrid::s4u::this_actor::sleep_for(150);

  XBT_INFO("The host is now up and running. Switch back to previous pstate %d", previous_pstate);
  host->set_pstate(previous_pstate);
}

static void simulate_shutdown(simgrid::s4u::Host* host)
{
  int previous_pstate = host->get_pstate();

  XBT_INFO("Switch to virtual pstate 4, that encodes the 'shutting down' state in that platform");
  host->set_pstate(4);

  XBT_INFO("Wait 7 seconds to simulate the shutdown time.");
  simgrid::s4u::this_actor::sleep_for(7);

  XBT_INFO("Switch back to previous pstate %d, that will be used on reboot.", previous_pstate);
  host->set_pstate(previous_pstate);

  XBT_INFO("Actually shutdown the host");
  host->turn_off();
}

static int monitor()
{
  simgrid::s4u::Host* host1 = simgrid::s4u::Host::by_name("MyHost1");

  XBT_INFO("Initial pstate: %d; Energy dissipated so far:%.0E J", host1->get_pstate(),
           sg_host_get_consumed_energy(host1));

  XBT_INFO("Sleep for 10 seconds");
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping. Current pstate: %d; Energy dissipated so far: %.2f J", host1->get_pstate(),
           sg_host_get_consumed_energy(host1));

  simulate_shutdown(host1);
  XBT_INFO("Host1 is now OFF. Current pstate: %d; Energy dissipated so far: %.2f J", host1->get_pstate(),
           sg_host_get_consumed_energy(host1));

  XBT_INFO("Sleep for 10 seconds");
  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Done sleeping. Current pstate: %d; Energy dissipated so far: %.2f J", host1->get_pstate(),
           sg_host_get_consumed_energy(host1));

  simulate_bootup(host1);
  XBT_INFO("Host1 is now ON again. Current pstate: %d; Energy dissipated so far: %.2f J", host1->get_pstate(),
           sg_host_get_consumed_energy(host1));

  return 0;
}

int main(int argc, char* argv[])
{
  sg_host_energy_plugin_init();
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("Boot Monitor", simgrid::s4u::Host::by_name("MyHost2"), monitor);

  e.run();

  XBT_INFO("End of simulation.");

  return 0;
}
