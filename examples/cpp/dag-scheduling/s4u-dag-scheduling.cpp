/* Copyright (c) 2009-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* simple test to schedule a DAX file with the Min-Min algorithm.           */
#include <algorithm>
#include <simgrid/host.h>
#include <simgrid/s4u.hpp>
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(dag_scheduling, "Logging specific to this example");
namespace sg4 = simgrid::s4u;

static std::vector<sg4::Exec*> get_ready_tasks(const std::vector<sg4::ActivityPtr>& dax)
{
  std::vector<sg4::Exec*> ready_tasks;
  std::map<sg4::Exec*, unsigned int> candidate_execs;

  for (const auto& a : dax) {
    // Only look at activity that have their dependencies solved but are not assigned
    if (a->dependencies_solved() && not a->is_assigned()) {
      // if it is an exec, it's ready
      if (auto* exec = dynamic_cast<sg4::Exec*>(a.get()))
        ready_tasks.push_back(exec);
      // if it a comm, we consider its successor as a candidate. If a candidate solves all its dependencies,
      // i.e., get all its input data, it's ready
      if (const auto* comm = dynamic_cast<sg4::Comm*>(a.get())) {
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

static sg4::Host* get_best_host(const sg4::ExecPtr exec, double* min_finish_time)
{
  sg4::Host* best_host = nullptr;
  *min_finish_time = std::numeric_limits<double>::max();

  for (const auto& host : sg4::Engine::get_instance()->get_all_hosts()) {
    double data_available      = 0.;
    double last_data_available = -1.0;
    /* compute last_data_available */
    for (const auto& parent : exec->get_dependencies()) {
      /* normal case */
      if (const auto* comm = dynamic_cast<sg4::Comm*>(parent.get())) {
        const auto* source = comm->get_source();
        XBT_DEBUG("transfer from %s to %s", source->get_cname(), host->get_cname());
        /* Estimate the redistribution time from this parent */
        double redist_time;
        if (comm->get_remaining() <= 1e-6) {
          redist_time = 0;
        } else {
          double bandwidth      = std::numeric_limits<double>::max();
          auto [links, latency] = source->route_to(host);
          for (auto const& link : links)
            bandwidth = std::min(bandwidth, link->get_bandwidth());

          redist_time = latency + comm->get_remaining() / bandwidth;
        }
        // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential
        // start time
        data_available = *comm->get_data<double>() + redist_time;
      }

      /* no transfer, control dependency */
      if (const auto* parent_exec = dynamic_cast<sg4::Exec*>(parent.get()))
        data_available = parent_exec->get_finish_time();

      if (last_data_available < data_available)
        last_data_available = data_available;
    }

    double finish_time = std::max(*host->get_data<double>(), last_data_available) +
                         exec->get_remaining() / host->get_speed();

    XBT_DEBUG("%s finishes on %s at %f", exec->get_cname(), host->get_cname(), finish_time);

    if (finish_time < *min_finish_time) {
      *min_finish_time = finish_time;
      best_host        = host;
    }
  }

  return best_host;
}

static void schedule_on(sg4::ExecPtr exec, sg4::Host* host, double busy_until = 0.0)
{
  exec->set_host(host);
  // We use the user data field to store up to when the host is busy
  delete host->get_data<double>(); // In case we're erasing a previous value
  host->set_data(new double(busy_until));
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

  sg4::Exec::on_completion_cb([](sg4::Exec const& exec) {
    // when an Exec completes, we need to set the potential start time of all its ouput comms
    for (const auto& succ : exec.get_successors()) {
      auto* comm = dynamic_cast<sg4::Comm*>(succ.get());
      if (comm != nullptr) {
        auto* finish_time = new double(exec.get_finish_time());
        // We use the user data field to store the finish time of the predecessor of the comm, i.e., its potential start
        // time
        comm->set_data(finish_time);
      }
    }
  });

  e.load_platform(argv[1]);

  /* Mark all hosts as sequential, as it ought to be in such a scheduling example.
   *
   * It means that the hosts can only compute one thing at a given time. If an execution already takes place on a given
   * host, any subsequently started execution will be queued until after the first execution terminates */
  for (auto const& host : e.get_all_hosts()) {
    host->set_concurrency_limit(1);
    host->set_data(new double(0.0));
  }
  /* load the DAX file */
  auto dax = sg4::create_DAG_from_DAX(argv[2]);

  /* Schedule the root first */
  double root_finish_time;
  auto* root = static_cast<sg4::Exec*>(dax.front().get());
  auto* host = get_best_host(root, &root_finish_time);
  schedule_on(root, host);

  e.run();

  while (not vetoed.empty()) {
    XBT_DEBUG("Start new scheduling round");
    /* Get the set of ready tasks */
    auto ready_tasks = get_ready_tasks(dax);
    vetoed.clear();

    if (ready_tasks.empty()) {
      /* there is no ready exec, let advance the simulation */
      e.run();
      continue;
    }
    /* For each ready exec:
     * get the host that minimizes the completion time.
     * select the exec that has the minimum completion time on its best host.
     */
    double min_finish_time   = std::numeric_limits<double>::max();
    sg4::Exec* selected_task = nullptr;
    sg4::Host* selected_host = nullptr;

    for (auto* exec : ready_tasks) {
      XBT_DEBUG("%s is ready", exec->get_cname());
      double finish_time;
      host = get_best_host(exec, &finish_time);
      if (finish_time < min_finish_time) {
        min_finish_time = finish_time;
        selected_task   = exec;
        selected_host   = host;
      }
    }

    XBT_INFO("Schedule %s on %s", selected_task->get_cname(), selected_host->get_cname());
    schedule_on(selected_task, selected_host, min_finish_time);

    ready_tasks.clear();
    e.run();
  }

  /* Cleanup memory */
  for (auto const* h : e.get_all_hosts())
    delete h->get_data<double>();

  XBT_INFO("Simulation Time: %f", simgrid_get_clock());

  return 0;
}
