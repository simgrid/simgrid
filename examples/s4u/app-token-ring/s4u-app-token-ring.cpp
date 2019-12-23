/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
#include <algorithm>
#include <string>
#include <map>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_token_ring, "Messages specific for this s4u example");

class RelayRunner {
  size_t task_comm_size = 1000000; /* The token is 1MB long*/
  simgrid::s4u::Mailbox* my_mailbox;
  simgrid::s4u::Mailbox* neighbor_mailbox;
  unsigned int rank      = 0;

public:
  explicit RelayRunner() = default;

  void operator()()
  {
    try {
      rank = std::stoi(simgrid::s4u::this_actor::get_name());
    } catch (const std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Processes of this example must have a numerical name, not ") +
                                  ia.what());
    }
    my_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(rank));
    if (rank + 1 == simgrid::s4u::Engine::get_instance()->get_host_count())
      /* The last process, which sends the token back to rank 0 */
      neighbor_mailbox = simgrid::s4u::Mailbox::by_name("0");
    else
      /* The others processes send to their right neighbor (rank+1) */
      neighbor_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(rank + 1));

    if (rank == 0) {
      /* The root process (rank 0) first sends the token then waits to receive it back */
      XBT_INFO("Host \"%u\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox->get_cname());
      std::string msg = "Token";
      neighbor_mailbox->put(&msg, task_comm_size);
      const std::string* res = static_cast<std::string*>(my_mailbox->get());
      XBT_INFO("Host \"%u\" received \"%s\"", rank, res->c_str());
    } else {
      std::string* res = static_cast<std::string*>(my_mailbox->get());
      XBT_INFO("Host \"%u\" received \"%s\"", rank, res->c_str());
      XBT_INFO("Host \"%u\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox->get_cname());
      neighbor_mailbox->put(res, task_comm_size);
    }
  }
};

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  e.load_platform(argv[1]);

  XBT_INFO("Number of hosts '%zu'", e.get_host_count());
  int id = 0;
  std::vector<simgrid::s4u::Host*> list = e.get_all_hosts();
  for (auto const& host : list) {
    /* - Give a unique rank to each host and create a @ref relay_runner process on each */
    simgrid::s4u::Actor::create((std::to_string(id)).c_str(), host, RelayRunner());
    id++;
  }
  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
