/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

 #include "simgrid/s4u.hpp"
 #include "simgrid/plugins/load.h"
 #include <cstdlib>
 #include <iostream>

/* Main function of the process I want to start manually */
static int process_function(std::vector<std::string> args)
{
  simgrid::s4u::MailboxPtr mailbox = nullptr;
  simgrid::s4u::this_actor::execute(100);
  return 0;
}

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  std::vector<std::string> args;
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  e.loadPlatform(argv[1]); 
  
  simgrid::s4u::Actor::createActor("simple_func", simgrid::s4u::Host::by_name("Tremblay"), process_function, args);
  simgrid::s4u::Actor::createActor("simple_func", simgrid::s4u::Host::by_name("Fafard"), process_function, args);

  e.run();

  return 0;
}
