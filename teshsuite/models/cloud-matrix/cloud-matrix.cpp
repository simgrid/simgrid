/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Systematic test of the CPU sharing between VMs and PMs, and of the resulting energy consumption.
 *
 * It runs on the cluster_multi.xml platform. The PMs used are named after their amount of cores: pm1, pm2 and pm4 are
 * the first host of each cluster, computing 1Gf/s per core. The 'fast' PM is the second host of the 4-core cluster,
 * switched to its pstate computing 2Gf/s per core.
 *
 * Each scenario places some tasks on a PM, in VMs 'A', 'B' and 'C' of that PM, and on the 'fast' PM. It measures
 * the speed of each task and the power of each PM during a first phase, then possibly applies an event (an exec or an
 * actor moving between the PM and the VMs, VM A moving to the 'fast' PM, or the PM changing its speed), measures again
 * during a second phase, terminates all tasks in a given way, and finally checks that the PMs dissipate nothing
 * anymore.
 *
 * This program does not know the expected outcomes: it prints what it measures, one line per scenario. It is run by
 * the cloud-matrix.py script next to this file, which recomputes the expected outcome of each line on its own, and
 * compares on the fly. Run "cloud-matrix.py --help" for its usage.
 *
 * A scenario is described by a label such as "pm4 A2[ob] B[o] C[] P[oo] F[] ev=none end=cancel", meaning: a PM of 4
 * cores, a VM A of 2 cores containing an unbounded task 'o' and a task 'b' bounded to half a core, a VM B of 1 core
 * containing a task, an empty VM C of 1 core, 2 tasks on the PM, no task on the 'fast' PM, no event, and the tasks
 * get canceled at the end.
 * Task kinds: o (unbounded), b (bounded to 0.5 Gf/s), q (0.25 Gf/s), t (0.75 Gf/s), B (2 Gf/s, above the core speed),
 * and with 2 threads: d (unbounded), h (bounded to 1.5 Gf/s in total), l (bounded to 0.5 Gf/s in total).
 *
 * To debug a scenario, give a part of its label as second parameter (or use the --filter option of cloud-matrix.py):
 * only the matching scenarios are run. Give --list instead to list all scenarios. */

#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(cloud_matrix, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

constexpr double FLOPS      = 2e9;      // Amount of each task: no task completes before the end of the measures
constexpr double PHASE      = 0.25;     // Duration of each measure
constexpr double COMPLETION = 60;       // Time given to the tasks to complete, when the scenario lets them complete
constexpr double IDLE       = 1;        // Duration of the final check that nothing remains
constexpr size_t VM_RAM     = 12500000; // Moving it with a live migration takes 0.1s

const std::vector<std::string> pm_names           = {"pm1", "pm2", "pm4", "fast"};
const std::map<std::string, std::string> pm_hosts = {{"pm1", "node-0.1core.org"},
                                                     {"pm2", "node-0.2cores.org"},
                                                     {"pm4", "node-0.4cores.org"},
                                                     {"fast", "node-1.4cores.org"}};

static sg4::Host* get_pm(const std::string& name)
{
  return sg4::Host::by_name(pm_hosts.at(name));
}

struct Scenario {
  int pm_cores;
  int a_cores;
  std::string a_tasks; // Kinds of the tasks in VM A
  std::string b_tasks; // Kinds of the tasks in VM B (1 core)
  std::string c_tasks; // Kinds of the tasks in VM C (1 core)
  std::string pm_tasks; // Kinds of the tasks on the PM
  int fast_tasks;      // Amount of unbounded tasks on the 'fast' PM
  std::string event;   // none, exec_vm2pm, exec_pm2vm, exec_vm2vm, actor_vm2pm, actor_pm2vm, set_pm, live_migration,
                       // dvfs (the PM switches from 1 to 2Gf/s per core) or dvfs_down (from 2 to 1Gf/s per core)
  std::string end;     // cancel, kill, actor_end, complete_waited, complete_unwaited

  std::string label() const
  {
    return "pm" + std::to_string(pm_cores) + " A" + std::to_string(a_cores) + "[" + a_tasks + "] B[" + b_tasks +
           "] C[" + c_tasks + "] P[" + pm_tasks + "] F[" + std::string(fast_tasks, 'o') + "] ev=" + event +
           " end=" + end;
  }
};

struct Task {
  char kind;
  sg4::ActorPtr actor;
  sg4::ExecPtr exec;
};

static double bound_of(char kind)
{
  switch (kind) {
    case 'o':
      return -1;
    case 'b':
      return 0.5e9;
    case 'q':
      return 0.25e9;
    case 't':
      return 0.75e9;
    case 'B':
      return 2e9;
    case 'd':
      return -1;
    case 'h':
      return 1.5e9;
    case 'l':
      return 0.5e9;
    default:
      xbt_die("Unknown task kind '%c'", kind);
  }
}

static int threads_of(char kind)
{
  return std::string("dhl").find(kind) == std::string::npos ? 1 : 2;
}

static void task_actor(Task* task, const std::string& end, sg4::SemaphorePtr sem)
{
  sg4::ExecPtr exec = sg4::this_actor::exec_init(FLOPS); // per thread
  if (int threads = threads_of(task->kind); threads > 1)
    exec->set_thread_count(threads);
  if (double bound = bound_of(task->kind); bound > 0)
    exec->set_bound(bound);
  task->exec = exec;
  exec->start();

  if (end == "cancel") {
    try {
      exec->wait();
    } catch (const simgrid::CancelException&) {
      // Expected: the master cancels this exec
    }
  } else if (end == "kill" || end == "complete_waited") {
    exec->wait(); // Killed meanwhile, or the exec completes
  } else {        // actor_end and complete_unwaited: never wait for the exec, and terminate when the master says so
    sem->acquire();
  }
}

static std::vector<double> energies()
{
  std::vector<double> res;
  for (auto const& name : pm_names)
    res.push_back(sg_host_get_consumed_energy(get_pm(name)));
  return res;
}

/* Power of each PM (in W, that is in loaded cores) between two energy measures. Only non-null ones are shown. */
static std::string powers(const std::vector<double>& before, const std::vector<double>& after, double duration)
{
  std::string res;
  for (size_t i = 0; i < pm_names.size(); i++)
    if (double power = (after[i] - before[i]) / duration; std::abs(power) > 1e-6)
      res += " " + pm_names[i] + "=" + simgrid::xbt::string_printf("%.4f", power);
  return res.empty() ? " none" : res;
}

static std::vector<double> remaining(const std::vector<Task>& tasks)
{
  std::vector<double> res;
  for (auto const& task : tasks)
    res.push_back(task.exec->get_remaining());
  return res;
}

/* Speed of each task (in Gf/s) between two measures, grouped by initial location: A, B, C, P, F */
static std::string speeds(const Scenario& s, const std::vector<double>& before, const std::vector<double>& after,
                          double duration)
{
  std::string res;
  const std::vector<size_t> group_sizes = {s.a_tasks.size(), s.b_tasks.size(), s.c_tasks.size(), s.pm_tasks.size(),
                                           static_cast<size_t>(s.fast_tasks)};
  size_t i                              = 0;
  for (size_t g = 0; g < group_sizes.size(); g++) {
    res += g == 0 ? "" : ";";
    for (size_t j = 0; j < group_sizes[g]; j++, i++)
      res += (j == 0 ? "" : ",") + simgrid::xbt::string_printf("%.4f", (before[i] - after[i]) / duration / 1e9);
  }
  return res;
}

static void run_scenario(const Scenario& s)
{
  sg4::Host* pm   = get_pm("pm" + std::to_string(s.pm_cores));
  sg4::Host* fast = get_pm("fast");
  if (s.event == "dvfs_down")
    pm->set_pstate(1); // Before creating the VMs, so that they get created on a PM that is not in its default pstate
  auto* vm_a = pm->create_vm("A", s.a_cores, VM_RAM);
  auto* vm_b = pm->create_vm("B", 1, VM_RAM);
  auto* vm_c = pm->create_vm("C", 1, VM_RAM);
  vm_a->start();
  vm_b->start();
  vm_c->start();

  // Tasks of VM A, then of VM B, then of VM C, then of the PM, then of the 'fast' PM
  std::vector<Task> tasks;
  std::vector<sg4::Host*> hosts;
  for (char kind : s.a_tasks) {
    tasks.push_back({kind, nullptr, nullptr});
    hosts.push_back(vm_a);
  }
  for (char kind : s.b_tasks) {
    tasks.push_back({kind, nullptr, nullptr});
    hosts.push_back(vm_b);
  }
  for (char kind : s.c_tasks) {
    tasks.push_back({kind, nullptr, nullptr});
    hosts.push_back(vm_c);
  }
  for (char kind : s.pm_tasks) {
    tasks.push_back({kind, nullptr, nullptr});
    hosts.push_back(pm);
  }
  for (int i = 0; i < s.fast_tasks; i++) {
    tasks.push_back({'o', nullptr, nullptr});
    hosts.push_back(fast);
  }
  size_t first_a = 0;
  size_t first_p = s.a_tasks.size() + s.b_tasks.size() + s.c_tasks.size();

  sg4::SemaphorePtr sem = sg4::Semaphore::create(0);
  for (size_t i = 0; i < tasks.size(); i++) {
    Task* task     = &tasks[i];
    tasks[i].actor = hosts[i]->add_actor("task", [task, end = s.end, sem]() { task_actor(task, end, sem); });
  }

  // First phase. The tasks start their exec when the master sleeps, at the current date. Each thread computes FLOPS.
  std::vector<double> initial_amounts;
  for (auto const& task : tasks)
    initial_amounts.push_back(FLOPS * threads_of(task.kind));
  auto energy_start = energies();
  sg4::this_actor::sleep_for(PHASE);
  std::string line = s.label() + " | P1 " + speeds(s, initial_amounts, remaining(tasks), PHASE) + " W" +
                     powers(energy_start, energies(), PHASE);

  // Event, and second phase
  if (s.event != "none") {
    if (s.event == "exec_vm2pm")
      tasks[first_a].exec->set_host(pm);
    else if (s.event == "exec_pm2vm")
      tasks[first_p].exec->set_host(vm_a);
    else if (s.event == "exec_vm2vm")
      tasks[first_a].exec->set_host(vm_b);
    else if (s.event == "actor_vm2pm")
      tasks[first_a].actor->set_host(pm);
    else if (s.event == "actor_pm2vm")
      tasks[first_p].actor->set_host(vm_a);
    else if (s.event == "set_pm")
      vm_a->set_pm(fast);
    else if (s.event == "live_migration")
      sg_vm_migrate(vm_a, fast);
    else if (s.event == "dvfs")
      pm->set_pstate(1); // Only the PMs of 4 cores have this pstate, computing 2Gf/s per core
    else if (s.event == "dvfs_down")
      pm->set_pstate(0);
    else
      xbt_die("Unknown event %s", s.event.c_str());

    auto remaining_before = remaining(tasks);
    auto energy_before    = energies();
    sg4::this_actor::sleep_for(PHASE);
    line += " | P2 " + speeds(s, remaining_before, remaining(tasks), PHASE) + " W" +
            powers(energy_before, energies(), PHASE);
  }

  // Termination of all tasks
  if (s.end == "cancel") {
    for (auto const& task : tasks)
      task.exec->cancel();
  } else if (s.end == "kill") {
    for (auto const& task : tasks)
      task.actor->kill();
  } else if (s.end == "actor_end") {
    for (size_t i = 0; i < tasks.size(); i++)
      sem->release();
  } else if (s.end == "complete_waited") {
    sg4::this_actor::sleep_for(COMPLETION);
  } else if (s.end == "complete_unwaited") {
    sg4::this_actor::sleep_for(COMPLETION);
    for (size_t i = 0; i < tasks.size(); i++)
      sem->release();
  } else {
    xbt_die("Unknown end %s", s.end.c_str());
  }
  for (auto const& task : tasks)
    task.actor->join();

  // Nothing must remain
  auto energy_idle = energies();
  sg4::this_actor::sleep_for(IDLE);
  line += " | idle W" + powers(energy_idle, energies(), IDLE);

  XBT_INFO("%s", line.c_str());
  vm_a->destroy();
  vm_b->destroy();
  vm_c->destroy();
  pm->set_pstate(0);
}

/* All the multisets of at most 'max_size' task kinds taken in 'kinds' */
static std::vector<std::string> task_sets(const std::string& kinds, size_t max_size)
{
  std::vector<std::string> res = {""};
  for (size_t i = 0; i < res.size(); i++) // res grows while iterating
    if (res[i].size() < max_size)
      for (size_t k = res[i].empty() ? 0 : kinds.find(res[i].back()); k < kinds.size(); k++)
        res.push_back(res[i] + kinds[k]);
  return res;
}

static std::vector<Scenario> all_scenarios()
{
  std::vector<Scenario> res;
  const std::vector<std::string> moves      = {"exec_vm2pm", "exec_pm2vm", "exec_vm2vm", "actor_vm2pm", "actor_pm2vm"};
  const std::vector<std::string> vm_moves   = {"set_pm", "live_migration"};
  const std::vector<std::string> pm_changes = {"dvfs", "dvfs_down"};
  const std::vector<std::string> ends       = {"cancel", "kill", "actor_end", "complete_waited", "complete_unwaited"};

  // 1. Sharing: PM cores x VM cores x tasks in VM A (unbounded or bounded to half a core) x tasks in VMs B and C x
  //    tasks on the PM x a task on another PM. VMs B and C are alike, so only one of 'B[o] C[]' and 'B[] C[o]' is run.
  const std::vector<std::string> small_vm_tasks = {"", "o", "b"};
  for (int pm_cores : {1, 2, 4})
    for (int a_cores : {1, 2})
      if (a_cores <= pm_cores)
        for (auto const& a_tasks : task_sets("ob", 3))
          for (size_t b = 0; b < small_vm_tasks.size(); b++)
            for (size_t c = b; c < small_vm_tasks.size(); c++)
              for (int pm_tasks = 0; pm_tasks <= pm_cores + 1; pm_tasks++)
                for (int fast_tasks : {0, 1})
                  res.push_back({pm_cores, a_cores, a_tasks, small_vm_tasks[b], small_vm_tasks[c],
                                 std::string(pm_tasks, 'o'), fast_tasks, "none", "cancel"});

  // 2. Other bounds: below, around and above the core speed
  for (auto const& a_tasks : task_sets("obqtB", 2))
    if (a_tasks.find_first_of("qtB") != std::string::npos)
      for (std::string pm_tasks : {"", "o"})
        res.push_back({4, 2, a_tasks, "", "", pm_tasks, 0, "none", "cancel"});

  // 3. Multi-threaded tasks, bounded or not, directly on the PM and in a VM
  for (int pm_cores : {1, 2, 4})
    for (auto const& pm_tasks : task_sets("odhl", 3))
      if (pm_tasks.find_first_of("dhl") != std::string::npos)
        res.push_back({pm_cores, 1, "", "", "", pm_tasks, 0, "none", "cancel"});
  for (int pm_cores : {2, 4})
    for (int a_cores : {1, 2})
      for (auto const& a_tasks : task_sets("odhl", 2))
        if (a_tasks.find_first_of("dhl") != std::string::npos)
          for (std::string pm_tasks : {"", "o", "d"})
            res.push_back({pm_cores, a_cores, a_tasks, "", "", pm_tasks, 0, "none", "cancel"});

  // 4. Moves: an exec or an actor moving between the PM and the VMs, or VM A moving to the 'fast' PM. VM A may be
  //    empty when an exec or an actor moves into it. The tasks that move may be multi-threaded.
  for (int pm_cores : {2, 4})
    for (int a_cores : {1, 2})
      for (std::string a_tasks : {"", "o", "oo", "b", "ob", "d", "h"})
        for (std::string pm_tasks : {"o", "oo", "d", "h"}) {
          for (auto const& event : moves)
            if (not a_tasks.empty() || event.find("pm2vm") != std::string::npos)
              res.push_back({pm_cores, a_cores, a_tasks, "", "", pm_tasks, 0, event, "cancel"});
          if (a_tasks.empty())
            continue;
          for (auto const& event : vm_moves)
            for (int fast_tasks : {0, 1})
              res.push_back({pm_cores, a_cores, a_tasks, "", "", pm_tasks, fast_tasks, event, "cancel"});
          if (pm_cores == 4)
            for (auto const& event : pm_changes)
              res.push_back({pm_cores, a_cores, a_tasks, "", "", pm_tasks, 0, event, "cancel"});
        }

  // 5. Terminations, with and without moves or speed changes
  for (int a_cores : {1, 2})
    for (std::string a_tasks : {"", "o", "ob"})
      for (auto const& end : ends) {
        if (a_tasks.empty()) { // Only to see an exec or an actor moving into an empty VM, and terminating there
          for (auto const& event : moves)
            if (event.find("pm2vm") != std::string::npos)
              res.push_back({4, a_cores, a_tasks, "", "", "o", 0, event, end});
          continue;
        }
        res.push_back({4, a_cores, a_tasks, "", "", "o", 0, "none", end});
        for (auto const& event : moves)
          res.push_back({4, a_cores, a_tasks, "", "", "o", 0, event, end});
        for (auto const& event : vm_moves)
          res.push_back({4, a_cores, a_tasks, "", "", "o", 0, event, end});
        for (auto const& event : pm_changes)
          res.push_back({4, a_cores, a_tasks, "", "", "o", 0, event, end});
      }

  return res;
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc >= 2, "Usage: %s cluster_multi.xml [part of a scenario label | --list]", argv[0]);
  std::string filter = argc > 2 ? argv[2] : "";
  if (filter == "--list") { // Only list the scenarios
    for (auto const& scenario : all_scenarios())
      std::printf("%s\n", scenario.label().c_str());
    return 0;
  }

  sg_host_energy_plugin_init();
  sg_vm_live_migration_plugin_init();
  e.load_platform(argv[1]);
  get_pm("fast")->set_pstate(1);

  e.host_by_name("node-1.1core.org")->add_actor("master", [filter]() {
    int count = 0;
    for (auto const& scenario : all_scenarios())
      if (scenario.label().find(filter) != std::string::npos) {
        run_scenario(scenario);
        count++;
      }
    XBT_INFO("%d scenarios", count);
  });
  e.run();
  return 0;
}
