/* Copyright (c) 2018-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Actor.hpp>
#include <src/instr/instr_smpi.hpp>
#include <src/smpi/include/smpi_actor.hpp>
#include <src/smpi/include/smpi_comm.hpp>
#include <src/smpi/plugins/ampi/instr_ampi.hpp>
#include <xbt/replay.hpp>
#include <xbt/sysdep.h>

#include "ampi.hpp"
#include <smpi/sampi.h>

class AllocTracker {
  std::vector<size_t> memory_size_;
  std::map</*address*/ void*, size_t> alloc_table_; // Keep track of all allocations

public:
  void record(void* ptr, size_t size)
  {
    alloc_table_.emplace(ptr, size);
    if (not simgrid::s4u::this_actor::is_maestro()) {
      auto actor = static_cast<size_t>(simgrid::s4u::this_actor::get_pid());
      if (actor >= memory_size_.size())
        memory_size_.resize(actor + 1, 0);
      memory_size_[actor] += size;
    }
  }

  void forget(void* ptr)
  {
    auto elm = alloc_table_.find(ptr);
    xbt_assert(elm != alloc_table_.end());
    if (not simgrid::s4u::this_actor::is_maestro()) {
      auto actor = static_cast<size_t>(simgrid::s4u::this_actor::get_pid());
      memory_size_.at(actor) -= elm->second; // size
    }
    alloc_table_.erase(elm);
  }

  size_t summary() const
  {
    if (simgrid::s4u::this_actor::is_maestro())
      return 0;
    auto actor = static_cast<size_t>(simgrid::s4u::this_actor::get_pid());
    return actor < memory_size_.size() ? memory_size_[actor] : 0;
  }
};

static AllocTracker alloc_tracker;

extern "C" void* _sampi_malloc(size_t size)
{
  void* result = xbt_malloc(size);
  alloc_tracker.record(result, size);
  return result;
}

extern "C" void _sampi_free(void* ptr)
{
  alloc_tracker.forget(ptr);
  xbt_free(ptr);
}

extern "C" void* _sampi_calloc(size_t num_elm, size_t elem_size)
{
  size_t size  = num_elm * elem_size;
  void* result = xbt_malloc0(size);
  alloc_tracker.record(result, size);
  return result;
}
extern "C" void* _sampi_realloc(void* ptr, size_t size)
{
  alloc_tracker.forget(ptr);
  void* result = xbt_realloc(ptr, size);
  alloc_tracker.record(result, size);
  return result;
}

namespace simgrid {
namespace smpi {
namespace plugin {
namespace ampi {
xbt::signal<void(s4u::Actor const&)> on_iteration_in;
xbt::signal<void(s4u::Actor const&)> on_iteration_out;
} // namespace ampi
} // namespace plugin
} // namespace smpi
} // namespace simgrid

/* FIXME The following contains several times "rank() + 1". This works for one
 * instance, but we need to find a way to deal with this for several instances and
 * for daemons: If we just replace this with the process id, we will get id's that
 * don't start at 0 if we start daemons as well.
 */
int APMPI_Iteration_in(MPI_Comm comm)
{
  const SmpiBenchGuard suspend_bench;
  TRACE_Iteration_in(comm->rank() + 1, new simgrid::instr::NoOpTIData("iteration_in")); // implemented on instr_smpi.c
  return 1;
}

int APMPI_Iteration_out(MPI_Comm comm)
{
  const SmpiBenchGuard suspend_bench;
  TRACE_Iteration_out(comm->rank() + 1, new simgrid::instr::NoOpTIData("iteration_out"));
  return 1;
}

void APMPI_Migrate(MPI_Comm comm)
{
  const SmpiBenchGuard suspend_bench;
  TRACE_migration_call(comm->rank() + 1, new simgrid::instr::AmpiMigrateTIData(alloc_tracker.summary()));
}

int AMPI_Iteration_in(MPI_Comm comm)
{
  return APMPI_Iteration_in(comm);
}

int AMPI_Iteration_out(MPI_Comm comm)
{
  return APMPI_Iteration_out(comm);
}

void AMPI_Migrate(MPI_Comm comm)
{
  APMPI_Migrate(comm);
}
