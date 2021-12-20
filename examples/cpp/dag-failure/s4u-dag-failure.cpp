/* Copyright (c) 2006-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_failure, "Logging specific to this example");

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  simgrid::s4u::Engine::set_config("host/model:ptask_L07");
  e.load_platform(argv[1]);

  auto* faulty = e.host_by_name("Faulty Host");
  auto* safe   = e.host_by_name("Safe Host");
  simgrid::s4u::Exec::on_completion.connect([](simgrid::s4u::Exec const& exec) {
    if (exec.get_state() == simgrid::s4u::Activity::State::FINISHED)
      XBT_INFO("Activity '%s' is complete (start time: %f, finish time: %f)", exec.get_cname(), exec.get_start_time(),
               exec.get_finish_time());
    if (exec.get_state() == simgrid::s4u::Activity::State::FAILED)
      if (exec.is_parallel())
        XBT_INFO("Activity '%s' has failed. %.f %% remain to be done", exec.get_cname(),
                 100 * exec.get_remaining_ratio());
      else
        XBT_INFO("Activity '%s' has failed. %.f flops remain to be done", exec.get_cname(), exec.get_remaining());
  });

  /* creation of a single Exec that will poorly fail when the workstation will stop */
  XBT_INFO("First test: sequential Exec activity");
  simgrid::s4u::ExecPtr exec =
      simgrid::s4u::Exec::init()->set_name("Poor task")->set_flops_amount(2e10)->vetoable_start();

  XBT_INFO("Schedule Activity '%s' on 'Faulty Host'", exec->get_cname());
  exec->set_host(faulty);

  /* Add a child Exec that depends on the Poor task' */
  simgrid::s4u::ExecPtr child = simgrid::s4u::Exec::init()->set_name("Child")->set_flops_amount(2e10)->set_host(safe);
  exec->add_successor(child);
  child->vetoable_start();

  XBT_INFO("Run the simulation");
  e.run();

  XBT_INFO("let's unschedule Activity '%s' and reschedule it on the 'Safe Host'", exec->get_cname());
  exec->unset_host();
  exec->set_host(safe);

  XBT_INFO("Run the simulation again");
  e.run();

  XBT_INFO("Second test: parallel Exec activity");
  exec = simgrid::s4u::Exec::init()->set_name("Poor parallel task")->set_flops_amounts({2e10, 2e10})->vetoable_start();

  XBT_INFO("Schedule Activity '%s' on 'Safe Host' and 'Faulty Host'", exec->get_cname());
  exec->set_hosts({safe, faulty});

  /* Add a child Exec that depends on the Poor task' */
  child = simgrid::s4u::Exec::init()->set_name("Child")->set_flops_amount(2e10)->set_host(safe);
  exec->add_successor(child);
  child->vetoable_start();

  XBT_INFO("Run the simulation");
  e.run();

  XBT_INFO("let's unschedule Activity '%s' and reschedule it only on the 'Safe Host'", exec->get_cname());
  exec->unset_host();
  exec->set_flops_amount(4e10)->set_host(safe);

  XBT_INFO("Run the simulation again");
  e.run();

  return 0;
}
