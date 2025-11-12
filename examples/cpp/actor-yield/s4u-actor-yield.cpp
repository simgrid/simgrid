/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

/* This example does not much: It just spans over-polite actor that yield a large amount
 * of time before ending.
 *
 * This serves as an example for the sg4::this_actor::yield() function, with which an actor can request
 * to be rescheduled after the other actor that are ready at the current timestamp.
 *
 * It can also be used to benchmark our context-switching mechanism.
 */
XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor_yield, "Messages specific for this s4u example");

static void yielder(long number_of_yields)
{
  for (int i = 0; i < number_of_yields; i++)
    sg4::this_actor::yield();
  XBT_INFO("I yielded %ld times. Goodbye now!", number_of_yields);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);             /* Load the platform description */

  e.host_by_name("Tremblay")->add_actor("yielder", yielder, 10);
  e.host_by_name("Ruby")->add_actor("yielder", yielder, 15);

  e.run(); /* - Run the simulation */

  return 0;
}
