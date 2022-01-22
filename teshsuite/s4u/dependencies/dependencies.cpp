/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(dependencies, "Logging specific to this test");

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\nExample: %s two_clusters.xml", argv[0], argv[0]);
  e.load_platform(argv[1]);

  simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity const& activity) {
    const auto* exec = dynamic_cast<simgrid::s4u::Exec const*>(&activity);
    if (exec == nullptr) // Only Execs are concerned here
      return;
    XBT_INFO("Exec '%s' start time: %f, finish time: %f", exec->get_cname(), exec->get_start_time(),
             exec->get_finish_time());
  });

  /* creation of the activities and their dependencies */
  simgrid::s4u::ExecPtr A = simgrid::s4u::Exec::init()->set_name("A")->vetoable_start();
  simgrid::s4u::ExecPtr B = simgrid::s4u::Exec::init()->set_name("B")->vetoable_start();
  simgrid::s4u::ExecPtr C = simgrid::s4u::Exec::init()->set_name("C")->vetoable_start();
  simgrid::s4u::ExecPtr D = simgrid::s4u::Exec::init()->set_name("D")->vetoable_start();

  B->add_successor(A);
  C->add_successor(A);
  D->add_successor(B);
  D->add_successor(C);
  B->add_successor(C);

  try {
    A->add_successor(A);
    /* shouldn't work and must raise an exception */
    xbt_die("Hey, I can add a dependency between A and A!");
  } catch (const std::invalid_argument& e) {
    XBT_INFO("Caught attempt to self-dependency creation: %s", e.what());
  }

  try {
    B->add_successor(A); /* shouldn't work and must raise an exception */
    xbt_die("Oh oh, I can add an already existing dependency!");
  } catch (const std::invalid_argument& e) {
    XBT_INFO("Caught attempt to add an already existing dependency: %s", e.what());
  }

  try {
    A->remove_successor(C); /* shouldn't work and must raise an exception */
    xbt_die("Dude, I can remove an unknown dependency!");
  } catch (const std::invalid_argument& e) {
    XBT_INFO("Caught attempt to remove an unknown dependency: %s", e.what());
  }

  try {
    C->remove_successor(C); /* shouldn't work and must raise an exception */
    xbt_die("Wow, I can remove a dependency between Task C and itself!");
  } catch (const std::invalid_argument& e) {
    XBT_INFO("Caught attempt to remove a self-dependency: %s", e.what());
  }

  /* scheduling parameters */
  const auto hosts                           = e.get_all_hosts();
  std::vector<simgrid::s4u::Host*> host_list = {hosts[2], hosts[4]};
  std::vector<double> flops_amounts          = {2000000, 1000000};
  std::vector<double> bytes_amounts          = {0, 2000000, 3000000, 0};

  A->set_flops_amounts(flops_amounts)->set_bytes_amounts(bytes_amounts)->set_hosts(host_list);
  B->set_flops_amounts(flops_amounts)->set_bytes_amounts(bytes_amounts)->set_hosts(host_list);
  C->set_flops_amounts(flops_amounts)->set_bytes_amounts(bytes_amounts)->set_hosts(host_list);
  D->set_flops_amounts(flops_amounts)->set_bytes_amounts(bytes_amounts)->set_hosts(host_list);

  e.run();
  return 0;
}
