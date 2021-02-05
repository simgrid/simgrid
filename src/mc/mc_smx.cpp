/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"

#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_smx.hpp"

using simgrid::mc::remote;

/** HACK, Statically "upcast" a s_smx_actor_t into an ActorInformation
 *
 *  This gets 'actorInfo' from '&actorInfo->copy'. It upcasts in the
 *  sense that we could achieve the same thing by having ActorInformation
 *  inherit from s_smx_actor_t but we don't really want to do that.
 */
static inline simgrid::mc::ActorInformation* actor_info_cast(smx_actor_t actor)
{
  simgrid::mc::ActorInformation temp;
  std::size_t offset = (char*)temp.copy.get_buffer() - (char*)&temp;

  auto* process_info = reinterpret_cast<simgrid::mc::ActorInformation*>((char*)actor - offset);
  return process_info;
}

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

const char* MC_smx_actor_get_name(smx_actor_t actor)
{
  if (mc_model_checker == nullptr)
    return actor->get_cname();

  const simgrid::mc::RemoteSimulation* process = &mc_model_checker->get_remote_simulation();

  simgrid::mc::ActorInformation* info = actor_info_cast(actor);
  if (info->name.empty()) {
    simgrid::xbt::string_data string_data = simgrid::xbt::string::to_string_data(actor->name_);
    info->name = process->read_string(remote(string_data.data), string_data.len);
  }
  return info->name.c_str();
}

unsigned long MC_smx_get_maxpid()
{
  unsigned long maxpid;
  const char* name = "simgrid::kernel::actor::maxpid";
  if (mc_model_checker->get_remote_simulation().find_variable(name) == nullptr)
    name = "maxpid"; // We seem to miss the namespaces when compiling with GCC
  mc_model_checker->get_remote_simulation().read_variable(name, &maxpid, sizeof(maxpid));
  return maxpid;
}
