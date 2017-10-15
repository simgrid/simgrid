/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

static int executor(std::vector<std::string> /*args*/)
{
  /* this_actor::execute() tells SimGrid to pause the calling actor
   * until its host has computed the amount of flops passed as a parameter */
  simgrid::s4u::this_actor::execute(100);

  /* This simple example does not do anything beyond that */
  return 0;
}

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  std::vector<std::string> args;
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("executor", simgrid::s4u::Host::by_name("Tremblay"), executor, args);

  e.run();

  return 0;
}
