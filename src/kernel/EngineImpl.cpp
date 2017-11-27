/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"

namespace simgrid {
namespace kernel {

EngineImpl::EngineImpl() = default;
EngineImpl::~EngineImpl()
{
  sg_host_exit(); // Hosts should be part of the engine, at some point
  delete netRoot_;
  for (auto const& kv : netpoints_)
    delete kv.second;
}
}
}
