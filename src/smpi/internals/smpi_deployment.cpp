/* Copyright (c) 2004-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "SmpiHost.hpp"
#include "private.hpp"
#include "simgrid/msg.h" /* barrier */
#include "simgrid/s4u/Engine.hpp"
#include "smpi_comm.hpp"
#include <map>

namespace simgrid {
namespace smpi {
namespace app {

class Instance {
public:
  Instance(const char* name, int max_no_processes, int process_count, MPI_Comm comm, msg_bar_t finalization_barrier)
      : name(name)
      , size(max_no_processes)
      , present_processes(0)
      , comm_world(comm)
      , finalization_barrier(finalization_barrier)
  { }

  const char* name;
  int size;
  int present_processes;
  MPI_Comm comm_world;
  msg_bar_t finalization_barrier;
};
}
}
}

using simgrid::smpi::app::Instance;

static std::map<std::string, Instance> smpi_instances;
extern int process_count; // How many processes have been allocated over all instances?

/** \ingroup smpi_simulation
 * \brief Registers a running instance of a MPI program.
 *
 * FIXME : remove MSG from the loop at some point.
 * \param name the reference name of the function.
 * \param code the main mpi function (must have a int ..(int argc, char *argv[])) prototype
 * \param num_processes the size of the instance we want to deploy
 */
void SMPI_app_instance_register(const char *name, xbt_main_func_t code, int num_processes)
{
  if (code != nullptr) { // When started with smpirun, we will not execute a function
    SIMIX_function_register(name, code);
  }

  static int already_called = 0;
  if (not already_called) {
    already_called = 1;
    std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::getInstance()->getAllHosts();
    for (auto const& host : list) {
      host->extension_set(new simgrid::smpi::SmpiHost(host));
    }
  }

  Instance instance(name, num_processes, process_count, MPI_COMM_NULL, MSG_barrier_init(num_processes));
  MPI_Group group     = new simgrid::smpi::Group(instance.size);
  instance.comm_world = new simgrid::smpi::Comm(group, nullptr);
  MPI_Attr_put(instance.comm_world, MPI_UNIVERSE_SIZE, reinterpret_cast<void*>(instance.size));

  process_count+=num_processes;

  smpi_instances.insert(std::pair<std::string, Instance>(name, instance));
}

void smpi_deployment_register_process(const char* instance_id, int rank, simgrid::s4u::ActorPtr actor)
{
  if (smpi_instances.empty()) // no instance registered, we probably used smpirun.
    return;

  Instance& instance = smpi_instances.at(instance_id);

  instance.present_processes++;
  instance.comm_world->group()->set_mapping(actor, rank);
}

MPI_Comm* smpi_deployment_comm_world(const char* instance_id)
{
  if (smpi_instances.empty()) { // no instance registered, we probably used smpirun.
    return nullptr;
  }
  Instance& instance = smpi_instances.at(instance_id);
  return &instance.comm_world;
}

msg_bar_t smpi_deployment_finalization_barrier(const char* instance_id)
{
  if (smpi_instances.empty()) { // no instance registered, we probably used smpirun.
    return nullptr;
  }
  Instance& instance = smpi_instances.at(instance_id);
  return instance.finalization_barrier;
}

void smpi_deployment_cleanup_instances(){
  for (auto const& item : smpi_instances) {
    Instance instance = item.second;
    MSG_barrier_destroy(instance.finalization_barrier);
    simgrid::smpi::Comm::destroy(instance.comm_world);
  }
  smpi_instances.clear();
}
