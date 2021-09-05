/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETZONE_TEST_HPP
#define NETZONE_TEST_HPP

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"

// Callback function common to several routing unit tests
struct CreateHost {
  std::pair<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*>
  operator()(simgrid::s4u::NetZone* zone, const std::vector<unsigned long>& /*coord*/, unsigned long id) const
  {
    const simgrid::s4u::Host* host = zone->create_host(std::to_string(id), 1e9)->seal();
    return std::make_pair(host->get_netpoint(), nullptr);
  }
};

#endif
