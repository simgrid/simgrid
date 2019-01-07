/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-bittorrent.hpp"
#include "s4u-peer.hpp"
#include "s4u-tracker.hpp"

simgrid::xbt::Extension<simgrid::s4u::Host, HostBittorrent> HostBittorrent::EXTENSION_ID;

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file", argv[0]);

  e.load_platform(argv[1]);

  /* Install our extension on all existing hosts */
  HostBittorrent::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostBittorrent>();
  std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::get_instance()->get_all_hosts();
  for (auto const& host : list)
    host->extension_set(new HostBittorrent(host));

  e.register_actor<Tracker>("tracker");
  e.register_actor<Peer>("peer");
  e.load_deployment(argv[2]);

  e.run();

  return 0;
}
