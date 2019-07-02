/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/simdag.h"

#include <cmath>
#include <set>

XBT_LOG_NEW_DEFAULT_CATEGORY(sd_test, "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  sg_host_t host_list[2];
  double computation_amount[2];
  double communication_amount[4] = { 0 };

  /* initialization of SD */
  SD_init(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\nExample: %s two_clusters.xml", argv[0], argv[0]);
  SD_create_environment(argv[1]);

  /* test the estimation functions */
  const sg_host_t* hosts           = sg_host_list();
  simgrid::s4u::Host* h1           = hosts[4];
  simgrid::s4u::Host* h2           = hosts[2];
  double comp_amount1 = 2000000;
  double comp_amount2 = 1000000;
  double comm_amount12 = 2000000;
  double comm_amount21 = 3000000;
  XBT_INFO("Computation time for %f flops on %s: %f", comp_amount1, h1->get_cname(), comp_amount1 / h1->get_speed());
  XBT_INFO("Computation time for %f flops on %s: %f", comp_amount2, h2->get_cname(), comp_amount2 / h2->get_speed());

  XBT_INFO("Route between %s and %s:", h1->get_cname(), h2->get_cname());
  std::vector<sg_link_t> route;
  double latency = 0;
  h1->route_to(h2, route, &latency);

  for (auto const& link : route)
    XBT_INFO("   Link %s: latency = %f, bandwidth = %f", sg_link_name(link), sg_link_latency(link),
             sg_link_bandwidth(link));

  XBT_INFO("Route latency = %f, route bandwidth = %f", latency, sg_host_route_bandwidth(h1, h2));
  XBT_INFO("Communication time for %f bytes between %s and %s: %f", comm_amount12, h1->get_cname(), h2->get_cname(),
           sg_host_route_latency(h1, h2) + comm_amount12 / sg_host_route_bandwidth(h1, h2));
  XBT_INFO("Communication time for %f bytes between %s and %s: %f", comm_amount21, h2->get_cname(), h1->get_cname(),
           sg_host_route_latency(h2, h1) + comm_amount21 / sg_host_route_bandwidth(h2, h1));

  /* creation of the tasks and their dependencies */
  SD_task_t taskA = SD_task_create("Task A", NULL, 10.0);
  SD_task_t taskB = SD_task_create("Task B", NULL, 40.0);
  SD_task_t taskC = SD_task_create("Task C", NULL, 30.0);
  SD_task_t taskD = SD_task_create("Task D", NULL, 60.0);

  /* try to attach and retrieve user data to a task */
  SD_task_set_data(taskA, static_cast<void*>(&comp_amount1));
  if (fabs(comp_amount1 - (*(static_cast<double*>(SD_task_get_data(taskA))))) > 1e-12)
      XBT_ERROR("User data was corrupted by a simple set/get");

  SD_task_dependency_add(taskB, taskA);
  SD_task_dependency_add(taskC, taskA);
  SD_task_dependency_add(taskD, taskB);
  SD_task_dependency_add(taskD, taskC);
  SD_task_dependency_add(taskB, taskC);

  try {
    SD_task_dependency_add(taskA, taskA); /* shouldn't work and must raise an exception */
    xbt_die("Hey, I can add a dependency between Task A and Task A!");
  } catch (const std::invalid_argument& e) {
    XBT_DEBUG("Caught invalid_argument: %s", e.what());
  }

  try {
    SD_task_dependency_add(taskB, taskA); /* shouldn't work and must raise an exception */
    xbt_die("Oh oh, I can add an already existing dependency!");
  } catch (const std::invalid_argument& e) {
    XBT_DEBUG("Caught invalid_argument: %s", e.what());
  }

  try {
    SD_task_dependency_remove(taskA, taskC);    /* shouldn't work and must raise an exception */
    xbt_die("Dude, I can remove an unknown dependency!");
  } catch (const std::invalid_argument& e) {
    XBT_DEBUG("Caught invalid_argument: %s", e.what());
  }

  try {
    SD_task_dependency_remove(taskC, taskC);    /* shouldn't work and must raise an exception */
    xbt_die("Wow, I can remove a dependency between Task C and itself!");
  } catch (const std::invalid_argument& e) {
    XBT_DEBUG("Caught invalid_argument: %s", e.what());
  }

  /* if everything is ok, no exception is forwarded or rethrown by main() */

  /* watch points */
  SD_task_watch(taskD, SD_DONE);
  SD_task_watch(taskB, SD_DONE);
  SD_task_unwatch(taskD, SD_DONE);

  /* scheduling parameters */
  host_list[0] = h1;
  host_list[1] = h2;
  computation_amount[0] = comp_amount1;
  computation_amount[1] = comp_amount2;

  communication_amount[1] = comm_amount12;
  communication_amount[2] = comm_amount21;

  /* estimated time */
  XBT_INFO("Estimated time for '%s': %f", SD_task_get_name(taskD),
           SD_task_get_execution_time(taskD, 2, host_list, computation_amount, communication_amount));

  SD_task_schedule(taskA, 2, host_list, computation_amount, communication_amount, -1);
  SD_task_schedule(taskB, 2, host_list, computation_amount, communication_amount, -1);
  SD_task_schedule(taskC, 2, host_list, computation_amount, communication_amount, -1);
  SD_task_schedule(taskD, 2, host_list, computation_amount, communication_amount, -1);

  std::set<SD_task_t> *changed_tasks = simgrid::sd::simulate(-1.0);
  for (auto const& task : *changed_tasks) {
    XBT_INFO("Task '%s' start time: %f, finish time: %f", SD_task_get_name(task),
          SD_task_get_start_time(task), SD_task_get_finish_time(task));
  }

  XBT_DEBUG("Destroying tasks...");
  SD_task_destroy(taskA);
  SD_task_destroy(taskB);
  SD_task_destroy(taskC);
  SD_task_destroy(taskD);

  XBT_DEBUG("Tasks destroyed. Exiting SimDag...");
  xbt_free((sg_host_t*)hosts);
  return 0;
}
