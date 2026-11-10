/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check that a VM loads its PM according to the bounds of the execs it contains.
 *
 * The VM model computes how many cores of the PM a VM uses from the execs it contains. A bounded exec only uses the
 * fraction of a core given by its bound, so a VM containing a single exec bounded to half a core must load its PM by
 * half a core only, and three such execs need one core and a half.
 *
 * On the hosts used here, a core computes 1Gf/s and consumes 1W when loaded and 0W when idle. Each scenario starts
 * some execs of 0.1 Gf in a VM of 2 cores on a PM of 4 cores, and checks when the execs end and the energy
 * dissipated by the PM over one second. */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include <cmath>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

constexpr double FLOP_AMOUNT = 100e6; // 0.1s on a 1Gf core
constexpr double PERIOD      = 1.0;
int failed_test              = 0;

static double total_energy()
{
  double energy = 0;
  for (sg4::Host* host : sg4::Engine::get_instance()->get_all_hosts())
    if (dynamic_cast<sg4::VirtualMachine*>(host) == nullptr)
      energy += sg_host_get_consumed_energy(host);
  return energy;
}

/* Start one exec per given bound in a fresh VM (a bound of 0 means no bound), and check the duration of each exec and
 * the energy dissipated by the PM. */
static void run_scenario(const std::string& name, const std::vector<double>& bounds,
                         const std::vector<double>& expected_durations, double expected_energy)
{
  sg4::Host* pm       = sg4::Host::by_name("node-0.4cores.org");
  auto* vm            = pm->create_vm("VM0", 2);
  double start_energy = total_energy();
  double start        = sg4::Engine::get_clock();

  std::vector<sg4::ExecPtr> execs;
  for (double bound : bounds) {
    sg4::ExecPtr exec = sg4::Exec::init()->set_flops_amount(FLOP_AMOUNT)->set_host(vm);
    if (bound > 0)
      exec->set_bound(bound);
    execs.push_back(exec->start());
  }

  bool failed = false;
  for (size_t i = 0; i < execs.size(); i++) {
    execs[i]->wait();
    if (double duration = execs[i]->get_finish_time() - start; std::abs(duration - expected_durations[i]) > 0.001) {
      XBT_INFO("FAILED TEST: %s: exec #%zu took %.4fs instead of %.4fs", name.c_str(), i, duration,
               expected_durations[i]);
      failed = true;
    }
  }
  sg4::this_actor::sleep_until(start + PERIOD);

  if (double consumed = total_energy() - start_energy; std::abs(consumed - expected_energy) > 0.001) {
    XBT_INFO("FAILED TEST: %s: consumed %.4fJ instead of %.4fJ", name.c_str(), consumed, expected_energy);
    failed = true;
  }
  if (failed)
    failed_test++;
  else
    XBT_INFO("Passed: %s", name.c_str());
  vm->destroy();
}

static void master_main()
{
  XBT_INFO("### One exec bounded to half a core: 0.5 core during 0.2s");
  run_scenario("[b]2", {0.5e9}, {0.2}, 0.1);

  XBT_INFO("### Three execs bounded to half a core: 1.5 cores during 0.2s");
  run_scenario("[bbb]2", {0.5e9, 0.5e9, 0.5e9}, {0.2, 0.2, 0.2}, 0.3);

  XBT_INFO("### An exec bounded to half a core and an unbounded one: 1.5 cores during 0.1s, then 0.5 core during 0.1s");
  run_scenario("[bo]2", {0.5e9, 0}, {0.2, 0.1}, 0.2);

  XBT_INFO("### Two execs bounded to a quarter and three quarters of a core: 1 core during 0.13s, then 0.25 core");
  run_scenario("[bb]2 (1/4, 3/4)", {0.25e9, 0.75e9}, {0.4, 0.1 / 0.75}, 0.1 + 0.1);

  XBT_INFO("### An exec bounded above the core speed uses one core");
  run_scenario("[B]2", {2e9}, {0.1}, 0.1);

  XBT_INFO("### Five execs bounded to half a core need 2.5 cores, but the VM only has 2 cores");
  run_scenario("[bbbbb]2", {0.5e9, 0.5e9, 0.5e9, 0.5e9, 0.5e9}, {0.25, 0.25, 0.25, 0.25, 0.25}, 0.5);

  XBT_INFO("### Unbounded execs (control case): 2 cores during 0.1s");
  run_scenario("[oo]2", {0, 0}, {0.1, 0.1}, 0.2);

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
