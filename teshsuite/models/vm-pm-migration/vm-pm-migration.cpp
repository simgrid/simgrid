/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check that the execs of a VM run at the speed of its new PM once the VM moved (GitHub #207).
 *
 * When a VM moves to another PM, the execs it contains must continue at the speed of the destination PM, and the VM
 * must compete on the destination PM like the execs it contains. This is checked both with a live migration (that
 * suspends and resumes the VM) and with VirtualMachine::set_pm() (that moves the VM without suspending it).
 *
 * The 'slow' PM computes 1Gf/s per core and the 'fast' one 2Gf/s per core. Both have 2 cores dissipating 1W each when
 * loaded, and nothing when idle. Moving the VM takes one second in each scenario. */

#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include <cmath>
#include <functional>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

constexpr double PERIOD = 3.0; // Duration of each scenario, longer than the execs
int failed_test         = 0;

static bool check(const std::string& what, double value, double expected, const char* unit)
{
  if (std::abs(value - expected) > 0.001) {
    XBT_INFO("FAILED TEST: %s is %.4f%s instead of %.4f%s", what.c_str(), value, unit, expected, unit);
    return false;
  }
  return true;
}

/* Run a scenario: 'setup' starts the execs and returns them, 'move' moves the VM. Check when each exec ends, and the
 * energy dissipated by each PM. */
static void run_scenario(const std::string& name, int vm_cores, const std::vector<double>& vm_flops, double pm_flops,
                         const std::function<void(sg4::VirtualMachine*, sg4::Host*)>& move,
                         const std::vector<double>& expected_vm_ends, double expected_pm_end,
                         double expected_slow_energy, double expected_fast_energy)
{
  sg4::Host* slow = sg4::Host::by_name("slow");
  sg4::Host* fast = sg4::Host::by_name("fast");
  auto* vm        = slow->create_vm("VM0", vm_cores, 1250000000); // Its RAM takes one second to migrate
  vm->start();

  double start       = sg4::Engine::get_clock();
  double slow_energy = sg_host_get_consumed_energy(slow);
  double fast_energy = sg_host_get_consumed_energy(fast);

  std::vector<sg4::ExecPtr> execs;
  for (double flops : vm_flops)
    execs.push_back(sg4::Exec::init()->set_flops_amount(flops)->set_host(vm)->start());
  sg4::ExecPtr pm_exec;
  if (pm_flops > 0)
    pm_exec = sg4::Exec::init()->set_flops_amount(pm_flops)->set_host(fast)->start();

  move(vm, fast);

  bool passed = true;
  for (size_t i = 0; i < execs.size(); i++) {
    execs[i]->wait();
    passed &= check(name + ": end of VM exec #" + std::to_string(i), execs[i]->get_finish_time() - start,
                    expected_vm_ends[i], "s");
  }
  if (pm_exec) {
    pm_exec->wait();
    passed &= check(name + ": end of PM exec", pm_exec->get_finish_time() - start, expected_pm_end, "s");
  }
  sg4::this_actor::sleep_until(start + PERIOD);
  passed &=
      check(name + ": energy of slow", sg_host_get_consumed_energy(slow) - slow_energy, expected_slow_energy, "J");
  passed &=
      check(name + ": energy of fast", sg_host_get_consumed_energy(fast) - fast_energy, expected_fast_energy, "J");

  if (passed)
    XBT_INFO("Passed: %s", name.c_str());
  else
    failed_test++;
  vm->destroy();
}

static void live_migration(sg4::VirtualMachine* vm, sg4::Host* dst)
{
  sg_vm_migrate(vm, dst);
}

static void set_pm_after_one_second(sg4::VirtualMachine* vm, sg4::Host* dst)
{
  sg4::this_actor::sleep_for(1);
  vm->set_pm(dst);
}

static void master_main()
{
  /* 1 Gf done on 'slow' in one second, the remaining 2 Gf take one second on 'fast' */
  XBT_INFO("### Live migration of a VM running an exec of 3 Gf");
  run_scenario("live migration", 1, {3e9}, 0, live_migration, {2.0}, 0, 1.0, 1.0);

  XBT_INFO("### VirtualMachine::set_pm() on a VM running an exec of 3 Gf");
  run_scenario("set_pm", 1, {3e9}, 0, set_pm_after_one_second, {2.0}, 0, 1.0, 1.0);

  /* The VM execs do 1 Gf each on 'slow' in one second. Meanwhile, the PM exec does 2 Gf alone on 'fast'. Then the 3
   * execs share the 4 Gf/s of 'fast': 4/3 Gf/s each. The VM execs end 0.75s later, and the PM exec does the remaining
   * 1 Gf at 2 Gf/s. Energy of 'fast': 1 core for 1s, 2 cores for 0.75s, 1 core for 0.5s. */
  XBT_INFO("### VirtualMachine::set_pm() on a VM running 2 execs, to a PM running another exec");
  run_scenario("set_pm with competition", 2, {2e9, 2e9}, 4e9, set_pm_after_one_second, {1.75, 1.75}, 2.25, 2.0, 3.0);

  XBT_INFO("## %d test failed", failed_test);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg4::Engine::set_config("network/model:CM02"); // So that migrating the RAM of the VM takes exactly one second
  sg_vm_live_migration_plugin_init();
  sg_host_energy_plugin_init();
  e.load_platform(argv[1]);
  e.host_by_name("slow")->add_actor("master", master_main);
  e.run();
  return failed_test == 0 ? 0 : 1;
}
