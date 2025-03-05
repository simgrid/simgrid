/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "smpi/smpi.h"
#include <iostream>
#include <vector>

namespace sg4 = simgrid::s4u;

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);

  SMPI_init();

  e.load_platform(argv[1]);

  std::vector<simgrid::s4u::Host*> hosts;
  hosts.emplace_back(e.host_by_name("Tremblay"));
  hosts.emplace_back(e.host_by_name("Jupiter"));

  const std::string executable = "../privatization/privatization";

  std::vector<std::string> my_args = {"-s", "-long"};

  SMPI_executable_start(executable, hosts, my_args);

  e.run();

  SMPI_finalize();

  return 0;
}
