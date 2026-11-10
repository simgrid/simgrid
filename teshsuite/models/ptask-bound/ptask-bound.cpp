/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check that the ptask model honors the bounds requested by the user, on execs, parallel execs and communications.
 *
 * The bound of an exec is the maximal amount of flops computed per second, all hosts together, and the rate of a
 * communication is its maximal amount of bytes per second. The ptask model expresses the speed of its actions as the
 * fraction of their work done per second, so it must convert these bounds. The bounds must also survive changes of
 * the host speed (see GitHub #306).
 *
 * This runs on cluster_multi.xml, whose hosts compute 1Gf/s per core. Those of the 4-core cluster can also switch to
 * a pstate computing 2Gf/s per core. */

#include "simgrid/s4u.hpp"
#include <cmath>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(ptask_bound, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

static void check(const char* what, double start, double expected)
{
  double duration = sg4::Engine::get_clock() - start;
  if (std::abs(duration - expected) > 1e-3)
    XBT_INFO("FAILED: %s took %.4f s instead of %.4f s", what, duration, expected);
  else
    XBT_INFO("Passed: %s took %.4f s", what, duration);
}

static void tester()
{
  sg4::Host* host1   = sg4::Host::by_name("node-0.2cores.org");
  sg4::Host* host2   = sg4::Host::by_name("node-1.2cores.org");
  sg4::Host* dvfs    = sg4::Host::by_name("node-0.4cores.org");
  auto hosts         = std::vector<sg4::Host*>{host1, host2};
  static int payload = 42;
  double start       = sg4::Engine::get_clock();

  sg4::Exec::init()->set_flops_amount(1e9)->set_host(host1)->start()->wait();
  check("an exec of 1 Gf", start, 1);

  start = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amount(1e9)->set_bound(0.5e9)->set_host(host1)->start()->wait();
  check("an exec of 1 Gf bounded to 0.5 Gf/s", start, 2);

  start = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amounts({1e9, 1e9})->set_hosts(hosts)->start()->wait();
  check("a parallel exec of 1 Gf on each of 2 hosts", start, 1);

  start = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amounts({1e9, 1e9})->set_bound(1e9)->set_hosts(hosts)->start()->wait();
  check("a parallel exec of 1 Gf on each of 2 hosts, bounded to 1 Gf/s in total", start, 2);

  start = sg4::Engine::get_clock();
  sg4::Exec::init()->set_flops_amounts({1e9, 3e9})->set_bound(2e9)->set_hosts(hosts)->start()->wait();
  check("a parallel exec of 1 and 3 Gf on 2 hosts, bounded to 2 Gf/s in total", start, 3);

  // The communication is limited by its rate rather than by the bandwidth. Its latency adds a few milliseconds.
  start = sg4::Engine::get_clock();
  sg4::Mailbox::by_name("mailbox")->put_init(&payload, 1e6)->set_rate(1e5)->wait();
  double duration = sg4::Engine::get_clock() - start;
  if (duration < 10 || duration > 10.01)
    XBT_INFO("FAILED: a communication of 1 MB at 100 kB/s took %.4f s instead of 10 s (plus its latency)", duration);
  else
    XBT_INFO("Passed: a communication of 1 MB at 100 kB/s took 10 s (plus its latency)");

  // The host gets faster after one second, but the exec must remain bounded: 0.5 Gf done, 0.5 Gf to go at 0.5 Gf/s
  start             = sg4::Engine::get_clock();
  sg4::ExecPtr exec = sg4::Exec::init()->set_flops_amount(1e9)->set_bound(0.5e9)->set_host(dvfs)->start();
  sg4::this_actor::sleep_for(1);
  dvfs->set_pstate(1);
  exec->wait();
  check("an exec of 1 Gf bounded to 0.5 Gf/s, on a host getting faster after 1 s", start, 2);
  dvfs->set_pstate(0);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  e.host_by_name("node-0.2cores.org")->add_actor("tester", tester);
  e.host_by_name("node-1.2cores.org")->add_actor("receiver", [] { sg4::Mailbox::by_name("mailbox")->get<int>(); });
  e.run();
  return 0;
}
