/* Copyright (c) 2021-2025. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
#include <vector>
#include <iostream>

static void runner()
{
  const auto* e             = simgrid::s4u::Engine::get_instance();
  simgrid::s4u::Host* host0 = e->host_by_name("c1_0");
  simgrid::s4u::Host* host1 = e->host_by_name("c2_0");

  std::vector<double> comp = {1e6, 1e6};
  std::vector<double> comm = {1, 2, 3, 4};

  std::vector<simgrid::s4u::Host*> h1 = {host0, host1};
  simgrid::s4u::this_actor::parallel_execute(h1, comp, comm);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  simgrid::s4u::Engine::set_config("host/model:ptask_L07");

  xbt_assert(argc == 2,
             "\nUsage: %s platform_ok.xml\n"
             "\tor: %s platform_bad.xml\n",
             argv[0], argv[0]);

  const char* platform_file = argv[1];

  try {
    e.load_platform(platform_file);
    e.host_by_name("c1_0")->add_actor("actor", runner);
    e.run();
  } catch (const simgrid::AssertionError& e) {
    std::cout << e.what() << "\n";
  }

  return 0;
}
