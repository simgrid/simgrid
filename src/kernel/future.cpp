/* Copyright (c) 2016-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/functional.hpp>

#include <simgrid/kernel/future.hpp>

#include "src/kernel/EngineImpl.hpp"

namespace simgrid {
namespace kernel {

void FutureStateBase::schedule(simgrid::xbt::Task<void()>&& job) const
{
  EngineImpl::get_instance()->add_task(std::move(job));
}

} // namespace kernel
} // namespace simgrid
