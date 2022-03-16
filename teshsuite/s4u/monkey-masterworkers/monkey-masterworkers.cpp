/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This is a version of the masterworkers that (hopefully) survives to the chaos monkey.
 * It tests synchronous send/receive as well as synchronous computations.
 *
 * It is not written to be pleasant to read, but instead to resist the aggressions of the monkey:
 * - Workers keep going until after a global variable `todo` reaches 0.
 * - The master is a daemon that just sends infinitely tasks
 *   (simgrid simulations stop as soon as all non-daemon actors are done).
 * - The platform is created programmatically to remove path issues and control the problem size.
 *
 * Command-line configuration items:
 * - host-count: how many actors to start (including the master
 * - task-count: initial value of the `todo` global
 * - deadline: time at which the simulation is known to be failed (to detect infinite loops).
 *
 * See the simgrid-monkey script for more information.
 */

#include <simgrid/s4u.hpp>
#include <xbt/config.hpp>
#include <xbt/string.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static simgrid::config::Flag<int> cfg_host_count{"host-count", "Host count (master on one, workers on the others)", 3};
static simgrid::config::Flag<double> cfg_deadline{"deadline", "When to fail the simulation (infinite loop detection)",
                                                  120};
static simgrid::config::Flag<int> cfg_task_count{"task-count", "Amount of tasks that must be executed to succeed", 1};

int todo; // remaining amount of tasks to execute, a global variable
sg4::Mailbox* mailbox; // as a global to reduce the amount of simcalls during actor reboot

XBT_ATTRIB_NORETURN static void master()
{
  double comp_size = 1e6;
  long comm_size   = 1e6;
  bool rebooting   = sg4::Actor::self()->get_restart_count() > 0;

  XBT_INFO("Master %s", rebooting ? "rebooting" : "booting");
  if (not rebooting) // Starting for the first time
    sg4::this_actor::on_exit(
        [](bool forcefully) { XBT_INFO("Master dying %s.", forcefully ? "forcefully" : "peacefully"); });

  while (true) { // This is a daemon
    xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
               "Failed to run all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);

    auto payload = std::make_unique<double>(comp_size);
    try {
      XBT_INFO("Try to send a message");
      mailbox->put(payload.get(), comm_size, 10.0);
      payload.release();
    } catch (const simgrid::TimeoutException&) {
      XBT_INFO("Timeouted while sending a task");
    } catch (const simgrid::NetworkFailureException&) {
      XBT_INFO("Got a NetworkFailureException. Wait a second before starting again.");
      sg4::this_actor::sleep_for(1);
    }
  }
  THROW_IMPOSSIBLE;
}

static void worker(int id)
{
  bool rebooting = sg4::Actor::self()->get_restart_count() > 0;

  XBT_INFO("Worker %s", rebooting ? "rebooting" : "booting");
  if (not rebooting) // Starting for the first time
    sg4::this_actor::on_exit(
        [id](bool forcefully) { XBT_INFO("worker %d dying %s.", id, forcefully ? "forcefully" : "peacefully"); });

  while (todo > 0) {
    xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
               "Failed to run all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);
    try {
      XBT_INFO("Waiting a message on %s", mailbox->get_cname());
      auto payload = mailbox->get_unique<double>(10);
      xbt_assert(payload != nullptr, "mailbox->get() failed");
      double comp_size = *payload;

      XBT_INFO("Start execution...");
      sg4::this_actor::execute(comp_size);
      XBT_INFO("Execution complete.");
      todo--;
    } catch (const simgrid::TimeoutException&) {
      XBT_INFO("Timeouted while getting a task.");
    } catch (const simgrid::NetworkFailureException&) {
      XBT_INFO("Got a NetworkFailureException. Wait a second before starting again.");
      sg4::this_actor::sleep_for(1);
    }
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  auto* rootzone = sg4::create_full_zone("root");
  std::vector<sg4::Host*> worker_hosts;

  xbt_assert(cfg_host_count > 2, "You need at least 2 workers (i.e., 3 hosts) or the master will be auto-killed when "
                                 "the only worker gets killed.");
  sg4::Host* master_host = rootzone->create_host("lilibeth 0", 1e9); // Host where the master will stay
  for (int i = 1; i < cfg_host_count; i++) {
    auto hostname = std::string("lilibeth ") + std::to_string(i);
    auto* host    = rootzone->create_host(hostname, 1e9);
    sg4::LinkInRoute link(rootzone->create_link(hostname, "1MBps")->set_latency("24us")->seal());
    rootzone->add_route(master_host->get_netpoint(), host->get_netpoint(), nullptr, nullptr, {link}, true);
    worker_hosts.push_back(host);
  }
  rootzone->seal();

  sg4::Actor::create("master", master_host, master)->daemonize()->set_auto_restart(true);
  int id = 0;
  for (auto* h : worker_hosts) {
    sg4::Actor::create("worker", h, worker, id)->set_auto_restart(true);
    id++;
  }

  todo = cfg_task_count;
  xbt_assert(todo > 0, "Please give more than %d tasks to run", todo);
  mailbox = sg4::Mailbox::by_name("mailbox");

  e.run();

  XBT_INFO("WE SURVIVED!");
  return 0;
}
