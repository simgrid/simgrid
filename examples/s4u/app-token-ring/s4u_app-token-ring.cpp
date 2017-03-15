/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include <simgrid/s4u.hpp>
#include <algorithm>
#include <string>
#include <map>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_token_ring, "Messages specific for this s4u example");
// FIXME: The following duplicates the content of s4u::Host
extern std::map<std::string, simgrid::s4u::Host*> host_list;

class RelayRunner {
  size_t task_comm_size = 1000000; /* The token is 1MB long*/
  simgrid::s4u::MailboxPtr my_mailbox;
  simgrid::s4u::MailboxPtr neighbor_mailbox;
  unsigned int rank      = 0;

public:
  explicit RelayRunner() = default;

  void operator()()
  {
    rank = xbt_str_parse_int(simgrid::s4u::this_actor::name().c_str(),
                             "Any process of this example must have a numerical name, not %s");
    my_mailbox = simgrid::s4u::Mailbox::byName(std::to_string(rank));
    if (rank + 1 == host_list.size())
      /* The last process, which sends the token back to rank 0 */
      neighbor_mailbox = simgrid::s4u::Mailbox::byName("0");
    else
      /* The others processes send to their right neighbor (rank+1) */
      neighbor_mailbox = simgrid::s4u::Mailbox::byName(std::to_string(rank + 1));

    if (rank == 0) {
      /* The root process (rank 0) first sends the token then waits to receive it back */
      XBT_INFO("Host \"%u\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox->name());
      simgrid::s4u::this_actor::send(neighbor_mailbox, xbt_strdup("Token"), task_comm_size);
      char* res = static_cast<char*>(simgrid::s4u::this_actor::recv(my_mailbox));
      XBT_INFO("Host \"%u\" received \"%s\"", rank, res);
      xbt_free(res);
    } else {
      char* res = static_cast<char*>(simgrid::s4u::this_actor::recv(my_mailbox));
      XBT_INFO("Host \"%u\" received \"%s\"", rank, res);
      XBT_INFO("Host \"%u\" send 'Token' to Host \"%s\"", rank, neighbor_mailbox->name());
      simgrid::s4u::this_actor::send(neighbor_mailbox, res, task_comm_size);
    }
  }
};

int main(int argc, char** argv)
{
  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform.xml\n", argv[0]);
  e->loadPlatform(argv[1]);

  XBT_INFO("Number of hosts '%zu'", host_list.size());
  int id = 0;
  for (auto h : host_list) {
    simgrid::s4u::Host* host = h.second;
    /* - Give a unique rank to each host and create a @ref relay_runner process on each */
    simgrid::s4u::Actor::createActor((std::to_string(id)).c_str(), host, RelayRunner());
    id++;
  }
  e->run();
  XBT_INFO("Simulation time %g", e->getClock());
  return 0;
}
