/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example tests increasing and decreasing parallelism degree of Tasks.
 * First we increase and decrease parallelism degree while the Task is idle,
 * then we increase and decrease parallelism degree while the Task has queued firings.
 */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_parallelism, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void manager(sg4::ExecTaskPtr t)
{
  t->set_parallelism_degree(1);
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(300);

  t->set_parallelism_degree(2);
  t->enqueue_firings(4);
  sg4::this_actor::sleep_for(300);

  t->set_parallelism_degree(1);
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(300);

  t->enqueue_firings(11);
  t->set_parallelism_degree(2);
  sg4::this_actor::sleep_for(150);
  t->set_parallelism_degree(1);
  sg4::this_actor::sleep_for(200);
  t->set_parallelism_degree(3);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  auto pm0 = e.host_by_name("PM0");
  auto t   = sg4::ExecTask::init("exec_A", 100 * pm0->get_speed(), pm0);
  sg4::Task::on_completion_cb(
      [](const sg4::Task* t) { XBT_INFO("Task %s finished (%d)", t->get_cname(), t->get_count()); });
  sg4::Task::on_start_cb([](const sg4::Task* t) { XBT_INFO("Task %s start", t->get_cname()); });
  sg4::Actor::create("sender", pm0, manager, t);

  e.run();
  return 0;
}
