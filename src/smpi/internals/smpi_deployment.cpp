/* Copyright (c) 2004-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_host.hpp"
#include "private.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Barrier.hpp"
#include "smpi_comm.hpp"
#include <map>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi);

namespace simgrid::smpi::app {

class Instance {
public:
  explicit Instance(int max_no_processes) : size_(max_no_processes)
  {
    auto* group = new simgrid::smpi::Group(size_);
    comm_world_ = new simgrid::smpi::Comm(group, nullptr, false, -1);
    bar_ = s4u::Barrier::create(size_);
  }
  s4u::BarrierPtr bar_;
  unsigned int size_;
  unsigned int finalized_ranks_ = 0;
  MPI_Comm comm_world_;
};
} // namespace simgrid::smpi::app

using simgrid::smpi::app::Instance;

static std::map<std::string, Instance, std::less<>> smpi_instances;

/** @ingroup smpi_simulation
 * @brief Registers a running instance of an MPI program.
 *
 * @param name the reference name of the function.
 * @param code either the main mpi function
 *             (must have a int ..(int argc, char *argv[]) prototype) or nullptr
 *             (if the function deployment is managed somewhere else —
 *              e.g., when deploying manually or using smpirun)
 * @param num_processes the size of the instance we want to deploy
 */
void SMPI_app_instance_register(const char *name, xbt_main_func_t code, int num_processes)
{
  if (code != nullptr) // When started with smpirun, we will not execute a function
    simgrid::s4u::Engine::get_instance()->register_function(name, code);

  smpi_instances.try_emplace(name, num_processes);
}
void SMPI_app_instance_start(const char* name, const std::function<void()>& code,
                             std::vector<simgrid::s4u::Host*> const& hosts)
{
  xbt_assert(not hosts.empty(), "Cannot start a SMPI instance on 0 hosts");

  auto [_, inserted] = smpi_instances.try_emplace(name, hosts.size());
  xbt_assert(inserted, "Cannot start two MPI applications of the same name '%s'", name);

  int rank = 0;
  for (auto* host : hosts) {
    auto rank_str = std::to_string(rank);
    auto actor    = simgrid::s4u::Actor::init(std::string(name) + "#" + rank_str, host);
    actor->set_property("instance_id", name);
    actor->set_property("rank", rank_str);
    actor->start(code);

    smpi_deployment_register_process(name, rank, actor.get());

    rank++;
  }
}
void SMPI_app_instance_join(const std::string& instance_id)
{
  std::vector<simgrid::s4u::ActorPtr> actors =
      simgrid::s4u::Engine::get_instance()->get_filtered_actors([instance_id](simgrid::s4u::ActorPtr act) {
        auto* actor_instance = act->get_property("instance_id");
        return actor_instance != nullptr && strcmp(actor_instance, instance_id.c_str()) == 0;
      });

  for (auto& act : actors)
    act->join();
}

void smpi_deployment_register_process(const std::string& instance_id, int rank, const simgrid::s4u::Actor* actor)
{
  const Instance& instance = smpi_instances.at(instance_id);
  instance.comm_world_->group()->set_mapping(actor->get_pid(), rank);
}

void smpi_deployment_startup_barrier(const std::string& instance_id)
{
  const Instance& instance = smpi_instances.at(instance_id);
  instance.bar_->wait();
}

void smpi_deployment_unregister_process(const std::string& instance_id)
{
  Instance& instance = smpi_instances.at(instance_id);
  instance.finalized_ranks_++;

  if (instance.finalized_ranks_ == instance.size_) {
    simgrid::smpi::Comm::destroy(instance.comm_world_);
    smpi_instances.erase(instance_id);
  }
}

MPI_Comm* smpi_deployment_comm_world(const std::string& instance_id)
{
  Instance& instance = smpi_instances.at(instance_id);
  return &instance.comm_world_;
}

void smpi_deployment_cleanup_instances(){
  for (auto const& [name, instance] : smpi_instances) {
    XBT_INFO("Stalling SMPI instance: %s. Do all your MPI ranks call MPI_Finalize()?", name.c_str());
    simgrid::smpi::Comm::destroy(instance.comm_world_);
  }
  smpi_instances.clear();
}

/** @brief Auxiliary method to get list of hosts to deploy app */
static std::vector<simgrid::s4u::Host*> smpi_get_hosts(const simgrid::s4u::Engine* e, const std::string& hostfile)
{
  if (hostfile == "") {
    return e->get_all_hosts();
  }
  std::vector<simgrid::s4u::Host*> hosts;
  std::ifstream in(hostfile.c_str());
  xbt_assert(in, "smpirun: Cannot open the host file: %s", hostfile.c_str());
  std::string str;
  while (std::getline(in, str)) {
    if (not str.empty())
      hosts.emplace_back(e->host_by_name(str));
  }
  xbt_assert(not hosts.empty(), "smpirun: the hostfile '%s' is empty", hostfile.c_str());
  return hosts;
}

/** @brief Read replay configuration from file */
static std::vector<std::string> smpi_read_replay(const std::string& replayfile)
{
  std::vector<std::string> replay;
  if (replayfile == "")
    return replay;

  std::ifstream in(replayfile.c_str());
  xbt_assert(in, "smpirun: Cannot open the replay file: %s", replayfile.c_str());
  std::string str;
  while (std::getline(in, str)) {
    if (not str.empty())
      replay.emplace_back(str);
  }

  return replay;
}

/** @brief Build argument vector to pass to process */
static std::vector<std::string> smpi_deployment_get_args(int rank_id, const std::vector<std::string>& replay,
                                                         const std::vector<const char*>& run_args)
{
  std::vector<std::string> args{std::to_string(rank_id)};
  // pass arguments to process only if not a replay execution
  if (replay.empty())
    args.insert(args.end(), begin(run_args), end(run_args));
  /* one trace per process */
  if (replay.size() > 1)
    args.emplace_back(replay[rank_id]);
  return args;
}

/**
 * @brief Deploy an SMPI application from a smpirun call
 *
 * This used to be done at smpirun script, parsing either the hostfile or the platform XML.
 * If hostfile isn't provided, get the list of hosts from engine.
 */
int smpi_deployment_smpirun(simgrid::s4u::Engine* e, const std::string& hostfile, int np, const std::string& replayfile,
                            int map, const std::vector<const char*>& run_args)
{
  auto hosts     = smpi_get_hosts(e, hostfile);
  auto replay    = smpi_read_replay(replayfile);
  int hosts_size = static_cast<int>(hosts.size());
  if (np == 0)
    np = hosts_size;

  xbt_assert(np > 0, "Invalid number of process (np must be > 0). Check your np parameter, platform or hostfile");

  if (np > hosts_size) {
    XBT_INFO("You requested to use %d ranks, but there is only %d processes in your hostfile...", np, hosts_size);
  }

  for (int i = 0; i < np; i++) {
    simgrid::s4u::Host* host = hosts[i % hosts_size];
    std::string rank_id      = std::to_string(i);
    auto args                = smpi_deployment_get_args(i, replay, run_args);
    auto actor               = host->add_actor(rank_id, rank_id, args);
    /* keeping the same behavior as done in smpirun script, print mapping rank/process */
    if (map != 0) {
      XBT_INFO("[rank %d] -> %s", i, host->get_cname());
    }
    actor->set_property("instance_id", "smpirun");
    actor->set_property("rank", rank_id);
    if (not replay.empty())
      actor->set_property("smpi_replay", "true");
    /* shared trace file, set it to rank 0 */
    if (i == 0 && replay.size() == 1)
      actor->set_property("tracefile", replay[0]);
  }
  return np;
}


/** @brief Deploy an SMPI executable from within an S4U application */
void SMPI_executable_start(const std::string& executable, const std::vector<simgrid::s4u::Host*> &hosts, const std::vector<std::string> run_args)
{
  SMPI_executable_init(executable);

  int hosts_size = static_cast<int>(hosts.size());

  for (int i = 0; i < hosts_size; i++) {
    simgrid::s4u::Host* host = hosts[i];
    std::string rank_id      = std::to_string(i);
    std::vector<std::string> args{rank_id};
    args.insert(args.end(), run_args.begin(), run_args.end());
    auto actor = host->add_actor(rank_id, executable, args);
    actor->set_property("instance_id", "SMPI_app");
    actor->set_property("rank", rank_id);
  }

  SMPI_app_instance_register("SMPI_app", nullptr, hosts.size());
  MPI_COMM_WORLD = *smpi_deployment_comm_world("SMPI_app");
}
