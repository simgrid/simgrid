/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"

#include "src/mc/ModelChecker.hpp"
#include "src/mc/remote/RemoteSimulation.hpp"

using simgrid::mc::remote;
/** @file
 *  @brief (Cross-process, MCer/MCed) Access to SMX structures
 *
 *  We copy some C data structure from the MCed process in the MCer process.
 *  This is implemented by:
 *
 *   - `model_checker->process.smx_process_infos`
 *      (copy of `simix_global->process_list`);
 *
 *   - `model_checker->process.smx_old_process_infos`
 *      (copy of `simix_global->actors_to_destroy`);
 *
 *   - `model_checker->hostnames`.
 *
 * The process lists are currently refreshed each time MCed code is executed.
 * We don't try to give a persistent MCer address for a given MCed process.
 * For this reason, a MCer simgrid::mc::Process* is currently not reusable after
 * MCed code.
 */

/** Load the remote list of processes into a vector
 *
 *  @param process      MCed process
 *  @param target       Local vector (to be filled with copies of `s_smx_actor_t`)
 *  @param remote_dynar Address of the process dynar in the remote list
 */
static void MC_process_refresh_simix_actor_dynar(const simgrid::mc::RemoteSimulation* process,
                                                 std::vector<simgrid::mc::ActorInformation>& target,
                                                 simgrid::mc::RemotePtr<s_xbt_dynar_t> remote_dynar)
{
  target.clear();

  s_xbt_dynar_t dynar;
  process->read_bytes(&dynar, sizeof(dynar), remote_dynar);

  auto* data = static_cast<smx_actor_t*>(::operator new(dynar.elmsize * dynar.used));
  process->read_bytes(data, dynar.elmsize * dynar.used, simgrid::mc::RemotePtr<void>(dynar.data));

  // Load each element of the vector from the MCed process:
  for (unsigned int i = 0; i < dynar.used; ++i) {
    simgrid::mc::ActorInformation info;

    info.address  = simgrid::mc::RemotePtr<simgrid::kernel::actor::ActorImpl>(data[i]);
    info.hostname = nullptr;
    process->read_bytes(&info.copy, sizeof(info.copy), remote(data[i]));
    target.push_back(std::move(info));
  }
  ::operator delete(data);
}
namespace simgrid {
namespace mc {

void RemoteSimulation::refresh_simix()
{
  if (this->cache_flags_ & RemoteSimulation::cache_simix_processes)
    return;

  // TODO, avoid to reload `&simix_global`, `simix_global`, `*simix_global`

  static_assert(std::is_same<
      std::unique_ptr<simgrid::simix::Global>,
      decltype(simix_global)
    >::value, "Unexpected type for simix_global");
  static_assert(sizeof(simix_global) == sizeof(simgrid::simix::Global*),
    "Bad size for simix_global");

  RemotePtr<simgrid::simix::Global> simix_global_p{this->read_variable<simgrid::simix::Global*>("simix_global")};

  // simix_global = REMOTE(*simix_global)
  Remote<simgrid::simix::Global> simix_global =
    this->read<simgrid::simix::Global>(simix_global_p);

  MC_process_refresh_simix_actor_dynar(this, this->smx_actors_infos, remote(simix_global.get_buffer()->actors_vector));
  MC_process_refresh_simix_actor_dynar(this, this->smx_dead_actors_infos,
                                       remote(simix_global.get_buffer()->dead_actors_vector));

  this->cache_flags_ |= RemoteSimulation::cache_simix_processes;
}

}
}
