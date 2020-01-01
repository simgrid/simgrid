/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/load_balancer.h>
#include <simgrid/s4u.hpp>
#include <simgrid/smpi/replay.hpp>
#include <smpi/smpi.h>
#include <src/smpi/include/smpi_comm.hpp>
#include <src/smpi/include/smpi_actor.hpp>
#include <src/smpi/plugins/ampi/instr_ampi.hpp>
#include <src/smpi/plugins/ampi/ampi.hpp>
#include <xbt/replay.hpp>

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/smpi/plugins/load_balancer/load_balancer.hpp" // This is not yet ready to be public

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(plugin_load_balancer, smpi, "Logging specific to the SMPI load balancing plugin");

static simgrid::config::Flag<int> cfg_migration_frequency("smpi/plugin/lb/migration-frequency", {"smpi/plugin/lb/migration_frequency"},
    "After how many calls to the migration function should the migration be actually executed?", 10,
    [](double val){if (val != 10) sg_load_balancer_plugin_init();});

namespace simgrid {
namespace smpi {
namespace plugin {

static simgrid::plugin::loadbalancer::LoadBalancer lb;

class MigrateParser : public simgrid::smpi::replay::ActionArgParser {
public:
  double memory_consumption;
  void parse(simgrid::xbt::ReplayAction& action, const std::string&)
  {
    // The only parameter is the amount of memory used by the current process.
    CHECK_ACTION_PARAMS(action, 1, 0);
    memory_consumption = std::stod(action[2]);
  }
};

/* This function simulates what happens when the original application calls
 * (A)MPI_Migrate. It executes the load balancing heuristics, makes the necessary
 * migrations and updates the task mapping in the load balancer. 
 */
class MigrateAction : public simgrid::smpi::replay::ReplayAction<simgrid::smpi::plugin::MigrateParser> {
public:
  explicit MigrateAction() : ReplayAction("Migrate") {}
  void kernel(simgrid::xbt::ReplayAction&)
  {
    static std::map<simgrid::s4u::ActorPtr, int> migration_call_counter;
    static simgrid::s4u::Barrier smpilb_bar(smpi_get_universe_size());
    simgrid::s4u::Host* cur_host = simgrid::s4u::this_actor::get_host();
    simgrid::s4u::Host* migrate_to_host;

    TRACE_migration_call(my_proc_id, nullptr);

    // We only migrate every "cfg_migration_frequency"-times, not at every call
    migration_call_counter[simgrid::s4u::Actor::self()]++;
    if ((migration_call_counter[simgrid::s4u::Actor::self()] % simgrid::config::get_value<int>(cfg_migration_frequency.get_name())) != 0) {
      return;
    }

    // TODO cheinrich: Why do we need this barrier?
    smpilb_bar.wait();

    static bool was_executed = false;
    if (not was_executed) {
      was_executed = true;
      XBT_DEBUG("Process %li runs the load balancer", my_proc_id);
      smpi_bench_begin();
      lb.run();
      smpi_bench_end();
    }

    // This barrier is required to ensure that the mapping has been computed and is available
    smpilb_bar.wait();
    was_executed = false; // Must stay behind this barrier so that all processes have passed the if clause

    migrate_to_host = lb.get_mapping(simgrid::s4u::Actor::self());
    if (cur_host != migrate_to_host) { // Origin and dest are not the same -> migrate
      std::vector<simgrid::s4u::Host*> migration_hosts = {cur_host, migrate_to_host};
      std::vector<double> comp_amount                  = {0, 0};
      std::vector<double> comm_amount = {0, /*must not be 0*/ std::max(args.memory_consumption, 1.0), 0, 0};

      xbt_os_timer_t timer = smpi_process()->timer();
      xbt_os_threadtimer_start(timer);
      simgrid::s4u::this_actor::parallel_execute(migration_hosts, comp_amount, comm_amount);
      xbt_os_threadtimer_stop(timer);
      smpi_execute(xbt_os_timer_elapsed(timer));

      // Update the process and host mapping in SimGrid.
      XBT_DEBUG("Migrating process %li from %s to %s", my_proc_id, cur_host->get_cname(), migrate_to_host->get_cname());
      TRACE_smpi_process_change_host(my_proc_id, migrate_to_host);
      simgrid::s4u::this_actor::set_host(migrate_to_host);
    }

    smpilb_bar.wait();

    smpi_bench_begin();
  }
};

/******************************************************************************
 *         Code to include the duration of iterations in the trace.           *
 ******************************************************************************/

// FIXME Move declaration
XBT_PRIVATE void action_iteration_in(simgrid::xbt::ReplayAction& action);
void action_iteration_in(simgrid::xbt::ReplayAction& action)
{
  CHECK_ACTION_PARAMS(action, 0, 0)
  TRACE_Iteration_in(simgrid::s4u::this_actor::get_pid(), nullptr);
  simgrid::smpi::plugin::ampi::on_iteration_in(*MPI_COMM_WORLD->group()->actor(std::stol(action[0])));
}

XBT_PRIVATE void action_iteration_out(simgrid::xbt::ReplayAction& action);
void action_iteration_out(simgrid::xbt::ReplayAction& action)
{
  CHECK_ACTION_PARAMS(action, 0, 0)
  TRACE_Iteration_out(simgrid::s4u::this_actor::get_pid(), nullptr);
  simgrid::smpi::plugin::ampi::on_iteration_out(*MPI_COMM_WORLD->group()->actor(std::stol(action[0])));
}
}
}
}

/** @ingroup plugin_loadbalancer
 * @brief Initializes the load balancer plugin
 * @details The load balancer plugin supports several AMPI load balancers that move ranks
 * around, based on their host's load.
 */
void sg_load_balancer_plugin_init()
{
  static bool done = false;
  if (!done) {
    done = true;
    simgrid::s4u::Exec::on_completion.connect([](simgrid::s4u::Actor const& actor, simgrid::s4u::Exec const& exec) {
      simgrid::smpi::plugin::lb.record_actor_computation(actor, exec.get_cost());
    });

    xbt_replay_action_register(
        "migrate", [](simgrid::xbt::ReplayAction& action) { simgrid::smpi::plugin::MigrateAction().execute(action); });
    xbt_replay_action_register("iteration_in", simgrid::smpi::plugin::action_iteration_in);
    xbt_replay_action_register("iteration_out", simgrid::smpi::plugin::action_iteration_out);
  }
}
