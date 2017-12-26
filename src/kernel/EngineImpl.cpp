/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"

#include <algorithm>

namespace simgrid {
namespace kernel {

EngineImpl::EngineImpl() = default;
EngineImpl::~EngineImpl()
{
  /* copy all names to not modify the map while iterating over it.
   *
   * Plus, the hosts are destroyed in the lexicographic order to ensure
   * that the output is reproducible: we don't want to kill them in the
   * pointer order as it could be platform-dependent, which would break
   * the tests.
   */
  std::vector<std::string> names;
  for (auto const& kv : hosts_)
    names.push_back(kv.second->getName());

  std::sort(names.begin(), names.end());

  for (auto const& name : names)
    hosts_.at(name)->destroy();

  /* Also delete the other data */
  delete netRoot_;
  for (auto const& kv : netpoints_)
    delete kv.second;
}
}
}
