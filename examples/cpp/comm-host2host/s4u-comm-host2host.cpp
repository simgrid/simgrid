/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This simple example demonstrates the Comm::sento_init() Comm::sento_async() functions,
   that can be used to create a direct communication from one host to another without
   relying on the mailbox mechanism.

   There is not much to say, actually: The _init variant creates the communication and
   leaves it unstarted (in case you want to modify this communication before it starts),
   while the _async variant creates and start it. In both cases, you need to wait() it.

   It is mostly useful when you want to have a centralized simulation of your settings,
   with a central actor declaring all communications occurring on your distributed system.
  */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_host2host, "Messages specific for this s4u example");

static void sender(sg4::Host* h1, sg4::Host* h2, sg4::Host* h3, sg4::Host* h4)
{
  XBT_INFO("Send c12 with sendto_async(%s -> %s), and c34 with sendto_init(%s -> %s)", h1->get_cname(), h2->get_cname(),
           h3->get_cname(), h4->get_cname());

  auto c12 = sg4::Comm::sendto_async(h1, h2, 1.5e7); // Creates and start a direct communication

  auto c34 = sg4::Comm::sendto_init(h3, h4); // Creates but do not start another direct communication
  c34->set_remaining(1e7);                   // Specify the amount of bytes to exchange in this comm

  // You can also detach() communications that you never plan to test() or wait().
  // Here we create a communication that only slows down the other ones
  auto noise = sg4::Comm::sendto_init(h1, h2);
  noise->set_remaining(10000);
  noise->detach();

  XBT_INFO("After creation,  c12 is %s (remaining: %.2e bytes); c34 is %s (remaining: %.2e bytes)",
           c12->get_state_str(), c12->get_remaining(), c34->get_state_str(), c34->get_remaining());
  sg4::this_actor::sleep_for(1);
  XBT_INFO("One sec later,   c12 is %s (remaining: %.2e bytes); c34 is %s (remaining: %.2e bytes)",
           c12->get_state_str(), c12->get_remaining(), c34->get_state_str(), c34->get_remaining());
  c34->start();
  XBT_INFO("After c34->start,c12 is %s (remaining: %.2e bytes); c34 is %s (remaining: %.2e bytes)",
           c12->get_state_str(), c12->get_remaining(), c34->get_state_str(), c34->get_remaining());
  c12->wait();
  XBT_INFO("After c12->wait, c12 is %s (remaining: %.2e bytes); c34 is %s (remaining: %.2e bytes)",
           c12->get_state_str(), c12->get_remaining(), c34->get_state_str(), c34->get_remaining());
  c34->wait();
  XBT_INFO("After c34->wait, c12 is %s (remaining: %.2e bytes); c34 is %s (remaining: %.2e bytes)",
           c12->get_state_str(), c12->get_remaining(), c34->get_state_str(), c34->get_remaining());

  /* As usual, you don't have to explicitly start communications that were just init()ed.
     The wait() will start it automatically. */
  auto c14 = sg4::Comm::sendto_init(h1, h4);
  c14->set_remaining(100)->wait(); // Chaining 2 operations on this new communication
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Actor::create("sender", e.host_by_name("Boivin"), sender, e.host_by_name("Tremblay"), e.host_by_name("Jupiter"),
                     e.host_by_name("Fafard"), e.host_by_name("Ginette"));

  e.run();

  XBT_INFO("Total simulation time: %.3f", sg4::Engine::get_clock());

  return 0;
}
