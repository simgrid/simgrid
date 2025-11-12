/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(exec_suspend, "Messages specific for this task example");

namespace sg4 = simgrid::s4u;

static void suspend_activity_for(simgrid::s4u::ExecPtr activity, double seconds)
{
  activity->suspend();
  double remaining_computations_before_sleep = activity->get_remaining();
  sg4::this_actor::sleep_for(seconds);
  activity->resume();

  // Sanity check: remaining computations have to be the same before and after this period
  XBT_INFO("Activity hasn't advanced: %s. (remaining before sleep: %f, remaining after sleep: %f)",
           remaining_computations_before_sleep == activity->get_remaining() ? "yes" : "no",
           remaining_computations_before_sleep, activity->get_remaining());
}

static void executor()
{
  double computation_amount = sg4::this_actor::get_host()->get_speed();

  // Execution will take 3 seconds
  auto activity = sg4::this_actor::exec_init(3 * computation_amount);

  // Add functions to be called when the activity is resumed or suspended
  activity->on_this_suspend_cb(
      [](const sg4::Exec& t) { XBT_INFO("Task is suspended (remaining: %f)", t.get_remaining()); });

  activity->on_this_resume_cb(
      [](const sg4::Exec& t) { XBT_INFO("Task is resumed (remaining: %f)", t.get_remaining()); });

  // The first second of the execution
  activity->start();
  sg4::this_actor::sleep_for(1);

  // Suspend the activity for 10 seconds
  suspend_activity_for(activity, 10);

  // Run the activity for one second
  sg4::this_actor::sleep_for(1);

  // Suspend the activity for the second time
  suspend_activity_for(activity, 10);

  // Finish execution
  activity->wait();
  XBT_INFO("Finished");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("executor", executor);

  // Start the simulation
  e.run();
  return 0;
}
