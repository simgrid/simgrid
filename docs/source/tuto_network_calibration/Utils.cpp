/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "Utils.hpp"

void load_dahu_platform(const simgrid::s4u::Engine& e, double bw, double lat)
{
  auto* root = simgrid::s4u::create_star_zone("dahu");
  /* create the backbone link */
  const simgrid::s4u::Link* l_bb = root->create_link("backbone", bw)->set_latency(lat)->seal();

  const simgrid::s4u::LinkInRoute backbone{l_bb};
  /* create nodes */
  constexpr char preffix[] = "dahu-";
  constexpr char suffix[]  = ".grid5000.fr";
  for (int i = 0; i < 32; i++) {
    /* create host */
    const auto hostname            = preffix + std::to_string(i) + suffix;
    const simgrid::s4u::Host* host = root->create_host(hostname, 1)->seal();

    root->add_route(host->get_netpoint(), nullptr, nullptr, nullptr, {backbone}, true);
  }
  root->seal();
}