/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

class test {
  double computation_amount;
  double priority;

public:
  explicit test(std::vector<std::string> args)
{
  computation_amount = std::stod(args[1]);
  priority = std::stod(args[2]);
}
void operator()()
{
  XBT_INFO("Hello! Running an actor of size %g with priority %g", computation_amount, priority);
  simgrid::s4u::this_actor::execute(computation_amount, priority);

  XBT_INFO("Goodbye now!");
}
};

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);
  
  e.registerFunction<test>("test");

  e.loadPlatform(argv[1]);
  e.loadDeployment(argv[2]);

  e.run();

  return 0;
}
