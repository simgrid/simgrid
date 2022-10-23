/* Copyright (c) 2013-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SURF_NETWORK_SMPI_HPP
#define SIMGRID_SURF_NETWORK_SMPI_HPP

#include <xbt/base.h>

#include "network_cm02.hpp"

namespace simgrid::kernel::resource {

class XBT_PRIVATE NetworkSmpiModel : public NetworkCm02Model {
public:
  using NetworkCm02Model::NetworkCm02Model;

protected:
  void check_lat_factor_cb() override;
  void check_bw_factor_cb() override;
};
} // namespace simgrid::kernel::resource

#endif
