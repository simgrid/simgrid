/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/load_balancer.h>
#include <simgrid/s4u/Actor.hpp>
#include <src/instr/instr_smpi.hpp>
#include <src/smpi/include/smpi_actor.hpp>
#include <src/smpi/include/smpi_comm.hpp>
#include <src/smpi/plugins/ampi/instr_ampi.hpp>
#include <xbt/replay.hpp>
#include <xbt/sysdep.h>

#include "ampi.hpp"
#include <smpi/sampi.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(plugin_pampi, smpi, "Logging specific to the AMPI functions");

static std::vector<size_t> memory_size(500, 0); // FIXME cheinrich This needs to be dynamic
static std::map</*address*/ void*, size_t> alloc_table; // Keep track of all allocations

extern "C" void* _sampi_malloc(size_t size)
{
  void* result = xbt_malloc(size);
  alloc_table.insert({result, size});
  if (not simgrid::s4u::this_actor::is_maestro()) {
    memory_size[simgrid::s4u::this_actor::get_pid()] += size;
  }
  return result;
}

extern "C" void _sampi_free(void* ptr)
{
  size_t alloc_size = alloc_table.at(ptr);
  int my_proc_id    = simgrid::s4u::this_actor::get_pid();
  memory_size[my_proc_id] -= alloc_size;
  xbt_free(ptr);
}

extern "C" void* _sampi_calloc(size_t num_elm, size_t elem_size)
{
  void* result = xbt_malloc0(num_elm * elem_size);
  alloc_table.insert({result, num_elm * elem_size});
  if (not simgrid::s4u::this_actor::is_maestro()) {
    memory_size[simgrid::s4u::this_actor::get_pid()] += num_elm * elem_size;
  }
  return result;
}
extern "C" void* _sampi_realloc(void* ptr, size_t size)
{
  void* result = xbt_realloc(ptr, size);
  int old_size = alloc_table.at(ptr);
  alloc_table.erase(ptr);
  alloc_table.insert({result, size});
  if (not simgrid::s4u::this_actor::is_maestro()) {
    memory_size[simgrid::s4u::this_actor::get_pid()] += size - old_size;
  }
  return result;
}

namespace simgrid {
namespace smpi {
namespace plugin {
namespace ampi {
simgrid::xbt::signal<void(simgrid::s4u::Actor const&)> on_iteration_in;
simgrid::xbt::signal<void(simgrid::s4u::Actor const&)> on_iteration_out;
}
}
}
}

/* FIXME The following contains several times "rank() + 1". This works for one
 * instance, but we need to find a way to deal with this for several instances and
 * for daemons: If we just replace this with the process id, we will get id's that
 * don't start at 0 if we start daemons as well.
 */
int APMPI_Iteration_in(MPI_Comm comm)
{
  smpi_bench_end();
  TRACE_Iteration_in(comm->rank() + 1, new simgrid::instr::NoOpTIData("iteration_in")); // implemented on instr_smpi.c
  smpi_bench_begin();
  return 1;
}

int APMPI_Iteration_out(MPI_Comm comm)
{
  smpi_bench_end();
  TRACE_Iteration_out(comm->rank() + 1, new simgrid::instr::NoOpTIData("iteration_out"));
  smpi_bench_begin();
  return 1;
}

void APMPI_Migrate(MPI_Comm comm)
{
  smpi_bench_end();
  int my_proc_id = simgrid::s4u::this_actor::get_pid();
  TRACE_migration_call(comm->rank() + 1, new simgrid::instr::AmpiMigrateTIData(memory_size[my_proc_id]));
  smpi_bench_begin();
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
