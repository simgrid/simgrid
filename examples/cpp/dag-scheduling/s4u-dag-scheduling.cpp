/* Copyright (c) 2009-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* simple test to schedule a DAX file with the Min-Min algorithm.           */
#include <math.h>
#include <simgrid/host.h>
#include <simgrid/s4u.hpp>
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_scheduling, "Logging specific to this example");

struct HostAttribute {
  /* Earliest time at which a host is ready to execute a task */
  double available_at                     = 0.0;
  simgrid::s4u::Exec* last_scheduled_task = nullptr;
};

static double sg_host_get_available_at(const simgrid::s4u::Host* host)
{
  return static_cast<HostAttribute*>(host->get_data())->available_at;
}

static void sg_host_set_available_at(simgrid::s4u::Host* host, double time)
{
  auto* attr         = static_cast<HostAttribute*>(host->get_data());
  attr->available_at = time;
}

static simgrid::s4u::Exec* sg_host_get_last_scheduled_task(const simgrid::s4u::Host* host)
{
  return static_cast<HostAttribute*>(host->get_data())->last_scheduled_task;
}

static void sg_host_set_last_scheduled_task(simgrid::s4u::Host* host, simgrid::s4u::ExecPtr task)
{
  auto* attr                = static_cast<HostAttribute*>(host->get_data());
  attr->last_scheduled_task = task.get();
}

static bool dependency_exists(const simgrid::s4u::Exec* src, simgrid::s4u::Exec* dst)
{
  const auto& dependencies = src->get_dependencies();
  const auto& successors   = src->get_successors();
  return (std::find(successors.begin(), successors.end(), dst) != successors.end() ||
          dependencies.find(dst) != dependencies.end());
}

static std::vector<simgrid::s4u::Exec*> get_ready_tasks(const std::vector<simgrid::s4u::ActivityPtr>& dax)
{
  std::vector<simgrid::s4u::Exec*> ready_tasks;
  std::map<simgrid::s4u::Exec*, unsigned int> candidate_execs;

  for (auto& a : dax) {
    // Only loot at activity that have their dependencies solved but are not assigned
    if (a->dependencies_solved() && not a->is_assigned()) {
      // if it is an exec, it's ready
      auto* exec = dynamic_cast<simgrid::s4u::Exec*>(a.get());
      if (exec != nullptr)
        ready_tasks.push_back(exec);
      // if it a comm, we consider its successor as a candidate. If a candidate solves all its dependencies,
      // i.e., get all its input data, it's ready
      const auto* comm = dynamic_cast<simgrid::s4u::Comm*>(a.get());
      if (comm != nullptr) {
        auto* next_exec = static_cast<simgrid::s4u::Exec*>(comm->get_successors().front().get());
        candidate_execs[next_exec]++;
        if (next_exec->get_dependencies().size() == candidate_execs[next_exec])
          ready_tasks.push_back(next_exec);
      }
    }
  }
  XBT_DEBUG("There are %zu ready tasks", ready_tasks.size());
  return ready_tasks;
}

static double finish_on_at(const simgrid::s4u::ExecPtr task, const simgrid::s4u::Host* host)
{
  double result;

  const auto& parents = task->get_dependencies();

  if (not parents.empty()) {
    double data_available = 0.;
    double last_data_available;
    /* compute last_data_available */
    last_data_available = -1.0;
    for (const auto& parent : parents) {
      /* normal case */
      const auto* comm = dynamic_cast<simgrid::s4u::Comm*>(parent.get());
      if (comm != nullptr) {
        auto source = comm->get_source();
        XBT_DEBUG("transfer from %s to %s", source->get_cname(), host->get_cname());
        /* Estimate the redistribution time from this parent */
        double redist_time;
        if (comm->get_remaining() <= 1e-6) {
          redist_time = 0;
        } else {
          redist_time = sg_host_get_route_latency(source, host) +
                        comm->get_remaining() / sg_host_get_route_bandwidth(source, host);
        }
        // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential start
        // time
        data_available = *(static_cast<double*>(comm->get_data())) + redist_time;
      }

      const auto* exec = dynamic_cast<simgrid::s4u::Exec*>(parent.get());
      /* no transfer, control dependency */
      if (exec != nullptr) {
        data_available = exec->get_finish_time();
      }

      if (last_data_available < data_available)
        last_data_available = data_available;
    }

    result = fmax(sg_host_get_available_at(host), last_data_available) + task->get_remaining() / host->get_speed();
  } else
    result = sg_host_get_available_at(host) + task->get_remaining() / host->get_speed();

  return result;
}

static simgrid::s4u::Host* get_best_host(const simgrid::s4u::ExecPtr exec)
{
  std::vector<simgrid::s4u::Host*> hosts = simgrid::s4u::Engine::get_instance()->get_all_hosts();
  auto best_host                         = hosts.front();
  double min_EFT                         = finish_on_at(exec, best_host);

  for (const auto& h : hosts) {
    double EFT = finish_on_at(exec, h);
    XBT_DEBUG("%s finishes on %s at %f", exec->get_cname(), h->get_cname(), EFT);

    if (EFT < min_EFT) {
      min_EFT   = EFT;
      best_host = h;
    }
  }
  return best_host;
}

int main(int argc, char** argv)
{
  double min_finish_time            = -1.0;
  simgrid::s4u::Exec* selected_task = nullptr;
  simgrid::s4u::Host* selected_host = nullptr;

  simgrid::s4u::Engine e(&argc, argv);
  std::set<simgrid::s4u::Activity*> vetoed;
  e.track_vetoed_activities(&vetoed);

  simgrid::s4u::Activity::on_completion_cb([](simgrid::s4u::Activity& activity) {
    // when an Exec completes, we need to set the potential start time of all its ouput comms
    const auto* exec = dynamic_cast<simgrid::s4u::Exec*>(&activity);
    if (exec == nullptr) // Only Execs are concerned here
      return;
    for (const auto& succ : exec->get_successors()) {
      auto* comm = dynamic_cast<simgrid::s4u::Comm*>(succ.get());
      if (comm != nullptr) {
        auto* finish_time = new double(exec->get_finish_time());
        // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential start
        // time
        comm->set_data(finish_time);
      }
    }
  });

  e.load_platform(argv[1]);

  /*  Allocating the host attribute */
  unsigned long total_nhosts = e.get_host_count();
  const auto hosts          = e.get_all_hosts();
  std::vector<HostAttribute> host_attributes(total_nhosts);
  for (unsigned long i = 0; i < total_nhosts; i++)
    hosts[i]->set_data(&host_attributes[i]);

  /* load the DAX file */
  std::vector<simgrid::s4u::ActivityPtr> dax = simgrid::s4u::create_DAG_from_DAX(argv[2]);

  /* Schedule the root first */
  auto* root = static_cast<simgrid::s4u::Exec*>(dax.front().get());
  auto host  = get_best_host(root);
  root->set_host(host);
  // we can also set the source of all the output comms of the root node
  for (const auto& succ : root->get_successors()) {
    auto* comm = dynamic_cast<simgrid::s4u::Comm*>(succ.get());
    if (comm != nullptr)
      comm->set_source(host);
  }

  e.run();

  while (not vetoed.empty()) {
    XBT_DEBUG("Start new scheduling round");
    /* Get the set of ready tasks */
    auto ready_tasks = get_ready_tasks(dax);
    vetoed.clear();

    if (ready_tasks.empty()) {
      ready_tasks.clear();
      /* there is no ready task, let advance the simulation */
      e.run();
      continue;
    }
    /* For each ready task:
     * get the host that minimizes the completion time.
     * select the task that has the minimum completion time on its best host.
     */
    for (auto task : ready_tasks) {
      XBT_DEBUG("%s is ready", task->get_cname());
      host               = get_best_host(task);
      double finish_time = finish_on_at(task, host);
      if (min_finish_time < 0 || finish_time < min_finish_time) {
        min_finish_time = finish_time;
        selected_task   = task;
        selected_host   = host;
      }
    }

    XBT_INFO("Schedule %s on %s", selected_task->get_cname(), selected_host->get_cname());
    selected_task->set_host(selected_host);
    // we can also set the destination of all the input comms of the selected task
    for (const auto& pred : selected_task->get_dependencies()) {
      auto* comm = dynamic_cast<simgrid::s4u::Comm*>(pred.get());
      if (comm != nullptr) {
        comm->set_destination(selected_host);
        delete static_cast<double*>(comm->get_data());
      }
    }
    // we can also set the source of all the output comms of the selected task
    for (const auto& succ : selected_task->get_successors()) {
      auto* comm = dynamic_cast<simgrid::s4u::Comm*>(succ.get());
      if (comm != nullptr)
        comm->set_source(selected_host);
    }

    /*
     * tasks can be executed concurrently when they can by default.
     * Yet schedulers take decisions assuming that tasks wait for resource availability to start.
     * The solution (well crude hack is to keep track of the last task scheduled on a host and add a special type of
     * dependency if needed to force the sequential execution meant by the scheduler.
     * If the last scheduled task is already done, has failed or is a predecessor of the current task, no need for a
     * new dependency
     */

    auto last_scheduled_task = sg_host_get_last_scheduled_task(selected_host);
    if (last_scheduled_task && (last_scheduled_task->get_state() != simgrid::s4u::Activity::State::FINISHED) &&
        (last_scheduled_task->get_state() != simgrid::s4u::Activity::State::FAILED) &&
        not dependency_exists(sg_host_get_last_scheduled_task(selected_host), selected_task))
      last_scheduled_task->add_successor(selected_task);

    sg_host_set_last_scheduled_task(selected_host, selected_task);
    sg_host_set_available_at(selected_host, min_finish_time);

    ready_tasks.clear();
    /* reset the min_finish_time for the next set of ready tasks */
    min_finish_time = -1.;
    e.run();
  }

  XBT_INFO("Simulation Time: %f", simgrid_get_clock());

  return 0;
}
