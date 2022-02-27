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

static simgrid::config::Flag<int> cfg_host_count{"host-count", "Host count (master on one, workers on the others)", 2};
static simgrid::config::Flag<double> cfg_deadline{"deadline", "When to fail the simulation (infinite loop detection)",
                                                  120};
static simgrid::config::Flag<int> cfg_task_count{"task-count", "Amount of tasks that must be executed to succeed", 1};

int todo; // remaining amount of tasks to execute, a global variable

static void master(double comp_size, long comm_size)
{
  XBT_INFO("Master booting");
  sg4::Actor::self()->daemonize();

  auto mailbox = sg4::Mailbox::by_name("mailbox");
  while (true) { // This is a daemon
    xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
               "Failed to run all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);

    auto* payload = new double(comp_size);
    try {
      XBT_INFO("Try to send a message");
      mailbox->put(payload, comm_size, 10.0);
    } catch (const simgrid::TimeoutException&) {
      delete payload;
      XBT_INFO("Timeouted while sending a task");
    } catch (const simgrid::NetworkFailureException&) {
      delete payload;
      XBT_INFO("Network error while sending a task");
    }
  }
  THROW_IMPOSSIBLE;
}

static void worker(int id)
{
  XBT_INFO("Worker booting");
  sg4::Mailbox* mailbox = sg4::Mailbox::by_name("mailbox");
  while (todo > 0) {
    xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
               "Failed to run all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);
    try {
      XBT_INFO("Waiting a message on %s", mailbox->get_cname());
      auto payload = mailbox->get_unique<double>(10);
      xbt_assert(payload != nullptr, "mailbox->get() failed");
      double comp_size = *payload;
      if (comp_size < 0) { /* - Exit when -1.0 is received */
        XBT_INFO("I'm done. See you!");
        break;
      }
      /*  - Otherwise, process the task */
      XBT_INFO("Start execution...");
      sg4::this_actor::execute(comp_size);
      XBT_INFO("Execution complete.");
      todo--;
    } catch (const simgrid::TimeoutException&) {
      XBT_INFO("Timeouted while getting a task.");

    } catch (const simgrid::NetworkFailureException&) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    }
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  XBT_INFO("host count: %d ", (int)cfg_host_count);

  auto* rootzone = sg4::create_full_zone("root");
  sg4::Host* main; // First host created, where the master will stay
  std::vector<sg4::Host*> worker_hosts;
  for (int i = 0; i < cfg_host_count; i++) {
    auto hostname = std::string("lilibeth ") + std::to_string(i);
    auto* host    = rootzone->create_host(hostname, 1e15);
    if (i == 0) {
      main = host;
    } else {
      sg4::LinkInRoute link(rootzone->create_link(hostname, "1MBps")->set_latency("24us")->seal());
      rootzone->add_route(main->get_netpoint(), host->get_netpoint(), nullptr, nullptr, {link}, true);
      worker_hosts.push_back(host);
    }
  }
  rootzone->seal();
  sg4::Engine::get_instance()->on_platform_created(); // FIXME this should not be necessary

  sg4::Actor::create("master", main, master, 50000000, 1000000)->set_auto_restart(true);
  int id = 0;
  for (auto* h : worker_hosts)
    sg4::Actor::create("worker", h, worker, id++)->set_auto_restart(true);

  todo = cfg_task_count;
  xbt_assert(todo > 0, "Please give more than %d tasks to run", todo);

  e.run();

  XBT_INFO("WE SURVIVED!");
  return 0;
}
