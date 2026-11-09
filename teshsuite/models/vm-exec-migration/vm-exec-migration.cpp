/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check that the impact of a VM on its PM follows the execs that migrate in or out of that VM.
 *
 * The VM model counts the execs running in each VM to decide how many cores of the PM the VM uses. This count must be
 * updated when an exec migrates, either directly (Exec::set_host) or along with its actor (Actor::set_host). Otherwise,
 * a VM keeps loading its PM after its last exec left (wasting energy), or a VM receiving an exec does not get any core
 * of its PM to run it.
 *
 * On the hosts used here, a core computes 1Gf/s and consumes 1W when loaded and 0W when idle. Each scenario runs an
 * exec of 0.2 Gf that migrates after 0.1s, and then lets the platform idle for one second. The exec must take 0.2s,
 * and the PM must dissipate 0.2J over the whole scenario (one core during 0.2s): nothing during the idle period. */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

constexpr double FLOP_AMOUNT     = 200e6; // 0.2s on a 1Gf core
constexpr double MIGRATION_DELAY = 0.1;
constexpr double IDLE_PERIOD     = 1.0;
int failed_test                  = 0;

static double total_energy()
{
  double energy = 0;
  for (sg4::Host* host : sg4::Engine::get_instance()->get_all_hosts())
    if (dynamic_cast<sg4::VirtualMachine*>(host) == nullptr)
      energy += sg_host_get_consumed_energy(host);
  return energy;
}

static void check(const std::string& name, double start_energy, double exec_duration)
{
  if (std::abs(exec_duration - 0.2) > 0.001) {
    XBT_INFO("FAILED TEST: %s: the exec took %.4fs instead of 0.2s", name.c_str(), exec_duration);
    failed_test++;
  }
  sg4::this_actor::sleep_for(IDLE_PERIOD);
  double consumed = total_energy() - start_energy;
  if (std::abs(consumed - 0.2) > 0.001) {
    XBT_INFO("FAILED TEST: %s: consumed %.4fJ instead of 0.2J", name.c_str(), consumed);
    failed_test++;
  } else {
    XBT_INFO("Passed: %s", name.c_str());
  }
}

/* Start an exec on 'from', move it to 'to' after a while with Exec::set_host(), and wait for its completion */
static void test_exec_migration(const std::string& name, sg4::Host* from, sg4::Host* to)
{
  double start_energy = total_energy();
  double start        = sg4::Engine::get_clock();
  sg4::ExecPtr exec   = sg4::Exec::init()->set_flops_amount(FLOP_AMOUNT)->set_host(from)->start();
  sg4::this_actor::sleep_for(MIGRATION_DELAY);
  exec->set_host(to);
  exec->wait();
  check(name, start_energy, sg4::Engine::get_clock() - start);
}

/* Start an actor computing on 'from', move it to 'to' after a while with Actor::set_host(), and wait for its end */
static void test_actor_migration(const std::string& name, sg4::Host* from, sg4::Host* to)
{
  double start_energy  = total_energy();
  double start         = sg4::Engine::get_clock();
  double end           = -1;
  sg4::ActorPtr worker = from->add_actor("worker", [&end]() {
    sg4::this_actor::execute(FLOP_AMOUNT);
    end = sg4::Engine::get_clock();
  });
  sg4::this_actor::sleep_for(MIGRATION_DELAY);
  worker->set_host(to);
  worker->join();
  check(name, start_energy, end - start);
}

static void master_main()
{
  sg4::Host* pm = sg4::Host::by_name("node-0.2cores.org");

  XBT_INFO("### Exec::set_host(), from a VM to its PM");
  auto* vm = pm->create_vm("VM0", 2);
  test_exec_migration("Exec from VM to PM", vm, pm);
  vm->destroy();

  XBT_INFO("### Exec::set_host(), from a PM to one of its VMs");
  vm = pm->create_vm("VM0", 2);
  test_exec_migration("Exec from PM to VM", pm, vm);
  vm->destroy();

  XBT_INFO("### Exec::set_host(), between two VMs of the same PM");
  vm        = pm->create_vm("VM0", 2);
  auto* vm2 = pm->create_vm("VM1", 2);
  test_exec_migration("Exec from VM to VM", vm, vm2);
  vm->destroy();
  vm2->destroy();

  XBT_INFO("### Actor::set_host(), from a VM to its PM");
  vm = pm->create_vm("VM0", 2);
  test_actor_migration("Actor from VM to PM", vm, pm);
  vm->destroy();

  XBT_INFO("### Actor::set_host(), from a PM to one of its VMs");
  vm = pm->create_vm("VM0", 2);
  test_actor_migration("Actor from PM to VM", pm, vm);
  vm->destroy();

  XBT_INFO("## %d test failed", failed_test);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg_host_energy_plugin_init();
  e.load_platform(argv[1]);
  e.host_by_name("node-0.1core.org")->add_actor("master", master_main);
  e.run();
  return failed_test == 0 ? 0 : 1;
}
