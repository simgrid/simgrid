/* Copyright (c) 2007-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "simgrid/msg.h"
#include "simgrid/plugins/energy.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static void simulate_bootup(msg_host_t host) {
  int previous_pstate = MSG_host_get_pstate(host);

  XBT_INFO("Switch to virtual pstate 3, that encodes the shutting down state in the XML file of that example");
  MSG_host_set_pstate(host,3);

  msg_host_t host_list[1] = {host};
  double flops_amount[1] = {1};
  double bytes_amount[1] = {0};

  XBT_INFO("Actually start the host");
  MSG_host_on(host);

  XBT_INFO("Simulate the boot up by executing one flop on that host");
  // We use a parallel task to run some task on a remote host.
  msg_task_t bootup = MSG_parallel_task_create("boot up", 1, host_list, flops_amount, bytes_amount, NULL);
  MSG_task_execute(bootup);
  MSG_task_destroy(bootup);

  XBT_INFO("Switch back to previously selected pstate %d", previous_pstate);
  MSG_host_set_pstate(host, previous_pstate);
}

static void simulate_shutdown(msg_host_t host) {
  int previous_pstate = MSG_host_get_pstate(host);

  XBT_INFO("Switch to virtual pstate 4, that encodes the shutting down state in the XML file of that example");
  MSG_host_set_pstate(host,4);

  msg_host_t host_list[1] = {host};
  double flops_amount[1] = {1};
  double bytes_amount[1] = {0};

  XBT_INFO("Simulate the shutdown by executing one flop on that remote host (using a parallel task)");
  msg_task_t shutdown = MSG_parallel_task_create("shutdown", 1, host_list, flops_amount, bytes_amount, NULL);
  MSG_task_execute(shutdown);
  MSG_task_destroy(shutdown);

  XBT_INFO("Switch back to previously selected pstate %d", previous_pstate);
  MSG_host_set_pstate(host, previous_pstate);

  XBT_INFO("Actually shutdown the host");
  MSG_host_off(host);
}

static int onoff(int argc, char *argv[]) {
  msg_host_t host1 = MSG_host_by_name("MyHost1");

  XBT_INFO("Energetic profile: %s", MSG_host_get_property_value(host1,"watt_per_state"));
  XBT_INFO("Initial peak speed=%.0E flop/s; Energy dissipated =%.0E J",
     MSG_host_get_current_power_peak(host1), sg_host_get_consumed_energy(host1));

  XBT_INFO("Sleep for 10 seconds");
  MSG_process_sleep(10);
  XBT_INFO("Done sleeping. Current peak speed=%.0E; Energy dissipated=%.2f J",
     MSG_host_get_current_power_peak(host1), sg_host_get_consumed_energy(host1));

  simulate_shutdown(host1);
  XBT_INFO("Host1 is now OFF. Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
     MSG_host_get_current_power_peak(host1), sg_host_get_consumed_energy(host1));

  XBT_INFO("Sleep for 10 seconds");
  MSG_process_sleep(10);
  XBT_INFO("Done sleeping. Current peak speed=%.0E; Energy dissipated=%.2f J",
     MSG_host_get_current_power_peak(host1), sg_host_get_consumed_energy(host1));

  simulate_bootup(host1);
  XBT_INFO("Host1 is now ON again. Current peak speed=%.0E flop/s; Energy dissipated=%.0f J",
     MSG_host_get_current_power_peak(host1), sg_host_get_consumed_energy(host1));

  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  sg_energy_plugin_init();
  MSG_init(&argc, argv);

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
       "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_function_register("onoff_test", onoff);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Total simulation time: %.2f", MSG_get_clock());

  return res != MSG_OK;
}
