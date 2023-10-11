/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_dispatch, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void manager(sg4::ExecTaskPtr t)
{
  auto PM0 = sg4::Engine::get_instance()->host_by_name("PM0");
  auto PM1 = sg4::Engine::get_instance()->host_by_name("PM1");

  XBT_INFO("Test set_flops");
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(50);
  XBT_INFO("Set instance_0 flops to 50.");
  t->set_flops(50 * PM0->get_speed());
  sg4::this_actor::sleep_for(250);
  t->set_flops(100 * PM0->get_speed());

  XBT_INFO("Test set_parallelism degree");
  t->enqueue_firings(3);
  sg4::this_actor::sleep_for(50);
  XBT_INFO("Set Task parallelism degree to 2.");
  t->set_parallelism_degree(2);
  sg4::this_actor::sleep_for(250);
  t->set_parallelism_degree(1);

  XBT_INFO("Test set_host dispatcher");
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(50);
  XBT_INFO("Move dispatcher to PM1");
  t->set_host(PM1, "dispatcher");
  t->set_internal_bytes(1e6, "dispatcher");
  sg4::this_actor::sleep_for(250);
  t->set_host(PM0, "dispatcher");

  XBT_INFO("Test set_host instance_0");
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(50);
  XBT_INFO("Move instance_0 to PM1");
  t->set_host(PM1, "instance_0");
  t->set_flops(100 * PM1->get_speed());
  t->set_internal_bytes(1e6, "instance_0");
  sg4::this_actor::sleep_for(250);
  t->set_host(PM0, "instance_0");
  t->set_flops(100 * PM0->get_speed());

  XBT_INFO("Test set_host collector");
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(50);
  XBT_INFO("Move collector to PM1");
  t->set_host(PM1, "collector");
  sg4::this_actor::sleep_for(250);
  t->set_host(PM0, "collector");

  XBT_INFO("Test add_instances");
  t->enqueue_firings(1);
  sg4::this_actor::sleep_for(50);
  XBT_INFO("Add 1 instance and update load balancing function");
  t->add_instances(1);
  t->set_load_balancing_function([]() {
    static int round_robin_counter = 0;
    int ret                        = round_robin_counter;
    round_robin_counter            = round_robin_counter == 1 ? 0 : round_robin_counter + 1;
    return "instance_" + std::to_string(ret);
  });
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(250);

  XBT_INFO("Test remove_instances");
  XBT_INFO("Remove 1 instance and update load balancing function");
  t->remove_instances(1);
  t->set_load_balancing_function([]() { return "instance_0"; });
  t->enqueue_firings(2);
  sg4::this_actor::sleep_for(300);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  auto PM0 = e.host_by_name("PM0");
  auto PM1 = sg4::Engine::get_instance()->host_by_name("PM1");

  auto a = sg4::ExecTask::init("A", 100 * PM0->get_speed(), PM0);
  auto b = sg4::ExecTask::init("B", 50 * PM0->get_speed(), PM0);
  auto c = sg4::CommTask::init("C", 1e6, PM1, PM0);

  a->add_successor(b);

  sg4::Task::on_completion_cb(
      [](const sg4::Task* t) { XBT_INFO("Task %s finished (%d)", t->get_name().c_str(), t->get_count()); });
  sg4::Task::on_start_cb([](const sg4::Task* t) { XBT_INFO("Task %s start", t->get_name().c_str()); });

  sg4::Actor::create("manager", PM0, manager, a);

  e.run();
  return 0;
}
