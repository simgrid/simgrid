/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef NETZONE_TEST_HPP
#define NETZONE_TEST_HPP

#include "simgrid/s4u/NetZone.hpp"
#include "xbt/log.h"
XBT_LOG_EXTERNAL_CATEGORY(ker_platform);

// Callback function common to several routing unit tests
struct CreateHost {
  simgrid::s4u::Host* operator()(simgrid::s4u::NetZone* zone, const std::vector<unsigned long>& /*coord*/,
                                    unsigned long id) const
  {
    return zone->create_host(std::to_string(id), "1Gf");
  }
};

#endif
