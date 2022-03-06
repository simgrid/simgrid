/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* simple test to schedule a DAX file with the Min-Min algorithm.           */
#include <math.h>
#include <simgrid/host.h>
#include <simgrid/s4u.hpp>
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_scheduling, "Logging specific to this example");
namespace sg4 = simgrid::s4u;

struct HostAttribute {
  /* Earliest time at which a host is ready to execute a task */
  double available_at                     = 0.0;
  sg4::Exec* last_scheduled_task          = nullptr;
};

static double sg_host_get_available_at(const sg4::Host* host)
{
  return host->get_data<HostAttribute>()->available_at;
}

static void sg_host_set_available_at(const sg4::Host* host, double time)
{
  host->get_data<HostAttribute>()->available_at = time;
}

static sg4::Exec* sg_host_get_last_scheduled_task(const sg4::Host* host)
{
  return host->get_data<HostAttribute>()->last_scheduled_task;
}

static void sg_host_set_last_scheduled_task(const sg4::Host* host, sg4::ExecPtr task)
{
  host->get_data<HostAttribute>()->last_scheduled_task = task.get();
}

static bool dependency_exists(const sg4::Exec* src, sg4::Exec* dst)
{
  const auto& dependencies = src->get_dependencies();
  const auto& successors   = src->get_successors();
  return (std::find(successors.begin(), successors.end(), dst) != successors.end() ||
          dependencies.find(dst) != dependencies.end());
}

static std::vector<sg4::Exec*> get_ready_tasks(const std::vector<sg4::ActivityPtr>& dax)
{
  std::vector<sg4::Exec*> ready_tasks;
  std::map<sg4::Exec*, unsigned int> candidate_execs;

  for (auto& a : dax) {
    // Only loot at activity that have their dependencies solved but are not assigned
    if (a->dependencies_solved() && not a->is_assigned()) {
      // if it is an exec, it's ready
      auto* exec = dynamic_cast<sg4::Exec*>(a.get());
      if (exec != nullptr)
        ready_tasks.push_back(exec);
      // if it a comm, we consider its successor as a candidate. If a candidate solves all its dependencies,
      // i.e., get all its input data, it's ready
      const auto* comm = dynamic_cast<sg4::Comm*>(a.get());
      if (comm != nullptr) {
        auto* next_exec = static_cast<sg4::Exec*>(comm->get_successors().front().get());
        candidate_execs[next_exec]++;
        if (next_exec->get_dependencies().size() == candidate_execs[next_exec])
          ready_tasks.push_back(next_exec);
      }
    }
  }
  XBT_DEBUG("There are %zu ready tasks", ready_tasks.size());
  return ready_tasks;
}

static double finish_on_at(const sg4::ExecPtr task, const sg4::Host* host)
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
      const auto* comm = dynamic_cast<sg4::Comm*>(parent.get());
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
        data_available = *comm->get_data<double>() + redist_time;
      }

      const auto* exec = dynamic_cast<sg4::Exec*>(parent.get());
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

static sg4::Host* get_best_host(const sg4::ExecPtr exec)
{
  std::vector<sg4::Host*> hosts          = sg4::Engine::get_instance()->get_all_hosts();
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

static void schedule_on(sg4::ExecPtr exec, sg4::Host* host)
{
  exec->set_host(host);
  // we can also set the destination of all the input comms of this exec
  for (const auto& pred : exec->get_dependencies()) {
    auto* comm = dynamic_cast<sg4::Comm*>(pred.get());
    if (comm != nullptr) {
      comm->set_destination(host);
      delete comm->get_data<double>();
    }
  }
  // we can also set the source of all the output comms of this exec
  for (const auto& succ : exec->get_successors()) {
    auto* comm = dynamic_cast<sg4::Comm*>(succ.get());
    if (comm != nullptr)
      comm->set_source(host);
  }
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  std::set<sg4::Activity*> vetoed;
  e.track_vetoed_activities(&vetoed);

  sg4::Activity::on_completion_cb([](sg4::Activity const& activity) {
    // when an Exec completes, we need to set the potential start time of all its ouput comms
    const auto* exec = dynamic_cast<sg4::Exec const*>(&activity);
    if (exec == nullptr) // Only Execs are concerned here
      return;
    for (const auto& succ : exec->get_successors()) {
      auto* comm = dynamic_cast<sg4::Comm*>(succ.get());
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
  auto dax = sg4::create_DAG_from_DAX(argv[2]);

  /* Schedule the root first */
  auto* root = static_cast<sg4::Exec*>(dax.front().get());
  auto host  = get_best_host(root);
  schedule_on(root, host);

  e.run();

  while (not vetoed.empty()) {
    XBT_DEBUG("Start new scheduling round");
    /* Get the set of ready tasks */
    auto ready_tasks = get_ready_tasks(dax);
    vetoed.clear();

    if (ready_tasks.empty()) {
      /* there is no ready task, let advance the simulation */
      e.run();
      continue;
    }
    /* For each ready task:
     * get the host that minimizes the completion time.
     * select the task that has the minimum completion time on its best host.
     */
    double min_finish_time            = -1.0;
    sg4::Exec* selected_task          = nullptr;
    sg4::Host* selected_host          = nullptr;

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
    schedule_on(selected_task, selected_host);

    /*
     * tasks can be executed concurrently when they can by default.
     * Yet schedulers take decisions assuming that tasks wait for resource availability to start.
     * The solution (well crude hack is to keep track of the last task scheduled on a host and add a special type of
     * dependency if needed to force the sequential execution meant by the scheduler.
     * If the last scheduled task is already done, has failed or is a predecessor of the current task, no need for a
     * new dependency
     */

    auto last_scheduled_task = sg_host_get_last_scheduled_task(selected_host);
    if (last_scheduled_task && (last_scheduled_task->get_state() != sg4::Activity::State::FINISHED) &&
        (last_scheduled_task->get_state() != sg4::Activity::State::FAILED) &&
        not dependency_exists(sg_host_get_last_scheduled_task(selected_host), selected_task))
      last_scheduled_task->add_successor(selected_task);

    sg_host_set_last_scheduled_task(selected_host, selected_task);
    sg_host_set_available_at(selected_host, min_finish_time);

    ready_tasks.clear();
    e.run();
  }

  XBT_INFO("Simulation Time: %f", simgrid_get_clock());

  return 0;
}
