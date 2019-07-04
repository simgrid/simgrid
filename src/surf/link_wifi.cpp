/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/link_wifi.hpp"

namespace simgrid {
namespace kernel {
namespace resource {

void NetworkWifiLink::set_host_rate(sg_host_t host, int rate_level)
{
  host_rates.insert(std::make_pair(host->get_name(), rate_level));
}
} // namespace resource
} // namespace kernel
} // namespace simgrid
