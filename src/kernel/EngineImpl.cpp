/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/EngineImpl.hpp"
#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/sg_config.hpp"
#include "src/include/surf/surf.hpp" //get_clock() and surf_solve()
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/mc/mc_record.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf.hpp" // FIXME: KILLME. There must be a better way than mimicking XML here

XBT_LOG_NEW_DEFAULT_CATEGORY(ker_engine, "Logging specific to Engine (kernel)");

namespace simgrid {
namespace kernel {

config::Flag<double> cfg_breakpoint{"debug/breakpoint",
                                    "When non-negative, raise a SIGTRAP after given (simulated) time", -1.0};
EngineImpl::~EngineImpl()
{
  /* Since hosts_ is a std::map, the hosts are destroyed in the lexicographic order, which ensures that the output is
   * reproducible.
   */
  while (not hosts_.empty())
    hosts_.begin()->second->destroy();

  /* Also delete the other data */
  delete netzone_root_;
  for (auto const& kv : netpoints_)
    delete kv.second;

  for (auto const& kv : links_)
    if (kv.second)
      kv.second->destroy();
}

void EngineImpl::load_deployment(const std::string& file) const
{
  sg_platf_exit();
  sg_platf_init();

  surf_parse_open(file);
  surf_parse();
  surf_parse_close();
}

void EngineImpl::register_function(const std::string& name, const actor::ActorCodeFactory& code)
{
  registered_functions[name] = code;
}
void EngineImpl::register_default(const actor::ActorCodeFactory& code)
{
  default_function = code;
}

void EngineImpl::add_model(std::shared_ptr<resource::Model> model, const std::vector<resource::Model*>& dependencies)
{
  auto model_name = model->get_name();
  xbt_assert(models_prio_.find(model_name) == models_prio_.end(),
             "Model %s already exists, use model.set_name() to change its name", model_name.c_str());

  for (const auto dep : dependencies) {
    xbt_assert(models_prio_.find(dep->get_name()) != models_prio_.end(),
               "Model %s doesn't exists. Impossible to use it as dependency.", dep->get_name().c_str());
  }
  models_.push_back(model.get());
  models_prio_[model_name] = std::move(model);
}

/** Wake up all actors waiting for a Surf action to finish */
void EngineImpl::wake_all_waiting_actors() const
{
  for (auto const& model : models_) {
    XBT_DEBUG("Handling the failed actions (if any)");
    while (auto* action = model->extract_failed_action()) {
      XBT_DEBUG("   Handling Action %p", action);
      if (action->get_activity() != nullptr)
        activity::ActivityImplPtr(action->get_activity())->post();
    }
    XBT_DEBUG("Handling the terminated actions (if any)");
    while (auto* action = model->extract_done_action()) {
      XBT_DEBUG("   Handling Action %p", action);
      if (action->get_activity() == nullptr)
        XBT_DEBUG("probably vcpu's action %p, skip", action);
      else
        activity::ActivityImplPtr(action->get_activity())->post();
    }
  }
}

/** Execute all the tasks that are queued, e.g. `.then()` callbacks of futures. */
bool EngineImpl::execute_tasks()
{
  xbt_assert(tasksTemp.empty());

  if (tasks.empty())
    return false;

  do {
    // We don't want the callbacks to modify the vector we are iterating over:
    tasks.swap(tasksTemp);

    // Execute all the queued tasks:
    for (auto& task : tasksTemp)
      task();

    tasksTemp.clear();
  } while (not tasks.empty());

  return true;
}

void EngineImpl::rm_daemon(actor::ActorImpl* actor)
{
  auto it = daemons_.find(actor);
  xbt_assert(it != daemons_.end(), "The dying daemon is not a daemon after all. Please report that bug.");
  daemons_.erase(it);
}

void EngineImpl::run()
{
  if (MC_record_replay_is_active()) {
    mc::replay(MC_record_path());
    return;
  }

  double time = 0;

  do {
    XBT_DEBUG("New Schedule Round; size(queue)=%zu", simix_global->actors_to_run.size());

    if (cfg_breakpoint >= 0.0 && surf_get_clock() >= cfg_breakpoint) {
      XBT_DEBUG("Breakpoint reached (%g)", cfg_breakpoint.get());
      cfg_breakpoint = -1.0;
#ifdef SIGTRAP
      std::raise(SIGTRAP);
#else
      std::raise(SIGABRT);
#endif
    }

    execute_tasks();

    while (not simix_global->actors_to_run.empty()) {
      XBT_DEBUG("New Sub-Schedule Round; size(queue)=%zu", simix_global->actors_to_run.size());

      /* Run all actors that are ready to run, possibly in parallel */
      simix_global->run_all_actors();

      /* answer sequentially and in a fixed arbitrary order all the simcalls that were issued during that sub-round */

      /* WARNING, the order *must* be fixed or you'll jeopardize the simulation reproducibility (see RR-7653) */

      /* Here, the order is ok because:
       *
       *   Short proof: only maestro adds stuff to the actors_to_run array, so the execution order of user contexts do
       *   not impact its order.
       *
       *   Long proof: actors remain sorted through an arbitrary (implicit, complex but fixed) order in all cases.
       *
       *   - if there is no kill during the simulation, actors remain sorted according by their PID.
       *     Rationale: This can be proved inductively.
       *        Assume that actors_to_run is sorted at a beginning of one round (it is at round 0: the deployment file
       *        is parsed linearly).
       *        Let's show that it is still so at the end of this round.
       *        - if an actor is added when being created, that's from maestro. It can be either at startup
       *          time (and then in PID order), or in response to a process_create simcall. Since simcalls are handled
       *          in arbitrary order (inductive hypothesis), we are fine.
       *        - If an actor is added because it's getting killed, its subsequent actions shouldn't matter
       *        - If an actor gets added to actors_to_run because one of their blocking action constituting the meat
       *          of a simcall terminates, we're still good. Proof:
       *          - You are added from ActorImpl::simcall_answer() only. When this function is called depends on the
       *            resource kind (network, cpu, disk, whatever), but the same arguments hold. Let's take communications
       *            as an example.
       *          - For communications, this function is called from SIMIX_comm_finish().
       *            This function itself don't mess with the order since simcalls are handled in FIFO order.
       *            The function is called:
       *            - before the comm starts (invalid parameters, or resource already dead or whatever).
       *              The order then trivial holds since maestro didn't interrupt its handling of the simcall yet
       *            - because the communication failed or were canceled after startup. In this case, it's called from
       *              the function we are in, by the chunk:
       *                       set = model->states.failed_action_set;
       *                       while ((synchro = extract(set)))
       *                          SIMIX_simcall_post((smx_synchro_t) synchro->data);
       *              This order is also fixed because it depends of the order in which the surf actions were
       *              added to the system, and only maestro can add stuff this way, through simcalls.
       *              We thus use the inductive hypothesis once again to conclude that the order in which synchros are
       *              popped out of the set does not depend on the user code's execution order.
       *            - because the communication terminated. In this case, synchros are served in the order given by
       *                       set = model->states.done_action_set;
       *                       while ((synchro = extract(set)))
       *                          SIMIX_simcall_post((smx_synchro_t) synchro->data);
       *              and the argument is very similar to the previous one.
       *            So, in any case, the orders of calls to CommImpl::finish() do not depend on the order in which user
       *            actors are executed.
       *          So, in any cases, the orders of actors within actors_to_run do not depend on the order in which
       *          user actors were executed previously.
       *     So, if there is no killing in the simulation, the simulation reproducibility is not jeopardized.
       *   - If there is some actor killings, the order is changed by this decision that comes from user-land
       *     But this decision may not have been motivated by a situation that were different because the simulation is
       *     not reproducible.
       *     So, even the order change induced by the actor killing is perfectly reproducible.
       *
       *   So science works, bitches [http://xkcd.com/54/].
       *
       *   We could sort the actors_that_ran array completely so that we can describe the order in which simcalls are
       *   handled (like "according to the PID of issuer"), but it's not mandatory (order is fixed already even if
       *   unfriendly).
       *   That would thus be a pure waste of time.
       */

      for (auto const& actor : simix_global->actors_that_ran) {
        if (actor->simcall_.call_ != simix::Simcall::NONE) {
          actor->simcall_handle(0);
        }
      }

      execute_tasks();
      do {
        wake_all_waiting_actors();
      } while (execute_tasks());

      /* If only daemon actors remain, cancel their actions, mark them to die and reschedule them */
      if (simix_global->process_list.size() == daemons_.size())
        for (auto const& dmon : daemons_) {
          XBT_DEBUG("Kill %s", dmon->get_cname());
          simix_global->maestro_->kill(dmon);
        }
    }

    time = timer::Timer::next();
    if (time > -1.0 || not simix_global->process_list.empty()) {
      XBT_DEBUG("Calling surf_solve");
      time = surf_solve(time);
      XBT_DEBUG("Moving time ahead : %g", time);
    }

    /* Notify all the hosts that have failed */
    /* FIXME: iterate through the list of failed host and mark each of them */
    /* as failed. On each host, signal all the running actors with host_fail */

    // Execute timers and tasks until there isn't anything to be done:
    bool again = false;
    do {
      again = timer::Timer::execute_all();
      if (execute_tasks())
        again = true;
      wake_all_waiting_actors();
    } while (again);

    /* Clean actors to destroy */
    simix_global->empty_trash();

    XBT_DEBUG("### time %f, #actors %zu, #to_run %zu", time, simix_global->process_list.size(),
              simix_global->actors_to_run.size());

    if (time < 0. && simix_global->actors_to_run.empty() && not simix_global->process_list.empty()) {
      if (simix_global->process_list.size() <= daemons_.size()) {
        XBT_CRITICAL("Oops! Daemon actors cannot do any blocking activity (communications, synchronization, etc) "
                     "once the simulation is over. Please fix your on_exit() functions.");
      } else {
        XBT_CRITICAL("Oops! Deadlock or code not perfectly clean.");
      }
      simix_global->display_all_actor_status();
      simgrid::s4u::Engine::on_deadlock();
      for (auto const& kv : simix_global->process_list) {
        XBT_DEBUG("Kill %s", kv.second->get_cname());
        simix_global->maestro_->kill(kv.second);
      }
    }
  } while (time > -1.0 || not simix_global->actors_to_run.empty());

  if (not simix_global->process_list.empty())
    THROW_IMPOSSIBLE;

  simgrid::s4u::Engine::on_simulation_end();
}
} // namespace kernel
} // namespace simgrid
