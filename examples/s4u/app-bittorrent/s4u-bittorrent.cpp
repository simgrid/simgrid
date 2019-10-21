/* Copyright (c) 2012-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-bittorrent.hpp"
#include "s4u-peer.hpp"
#include "s4u-tracker.hpp"

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file", argv[0]);

  e.load_platform(argv[1]);

  e.register_actor<Tracker>("tracker");
  e.register_actor<Peer>("peer");
  e.load_deployment(argv[2]);

  e.run();

  return 0;
}
