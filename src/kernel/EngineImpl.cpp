/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/routing/AsImpl.hpp"
#include <simgrid/s4u/host.hpp>

namespace simgrid {
namespace kernel {

EngineImpl::EngineImpl()
{
}
EngineImpl::~EngineImpl()
{
  delete rootAs_;
}
}
}
