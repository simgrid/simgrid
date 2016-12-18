/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"
#include <simgrid/s4u/host.hpp>

xbt_dict_t netcards_dict;

namespace simgrid {
namespace kernel {

EngineImpl::EngineImpl()
{
  netcards_dict = xbt_dict_new_homogeneous([](void* p) {
    delete static_cast<simgrid::kernel::routing::NetCard*>(p);
  });
}
EngineImpl::~EngineImpl()
{
  delete netRoot_;
  xbt_dict_free(&netcards_dict);
}
}
}
