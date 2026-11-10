/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check that VMs follow the speed changes of their PM due to a speed profile.
 *
 * Two identical PMs of 2 cores get the same speed profile: full speed (1 Gf/s per core) during one second, then half
 * speed. The same tasks run directly on the first PM, and in a VM of 2 cores on the second one. As the VM is not
 * overloaded, both PMs must behave the same. The cloud-matrix test checks the speed changes due to DVFS, but cannot
 * check speed profiles, as a host accepts only one of them for the whole simulation.
 *
 * One task is unbounded, and the other one is bounded to 0.8 Gf/s: its bound is active at full speed only, so that
 * the VM must recompute the amount of cores it needs when the speed changes. */

#include "simgrid/kernel/ProfileBuilder.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(vm_speed_profile, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

constexpr double FLOPS = 4e9; // Long enough to last both phases

struct Measure {
  double unbounded;
  double bounded;
  double power;
};

/* Run the tasks on 'where', and measure their speeds and the power of 'pm' during both phases */
static std::pair<Measure, Measure> run_tasks(sg4::Host* pm, sg4::Host* where)
{
  sg4::ExecPtr unbounded = sg4::Exec::init()->set_flops_amount(FLOPS)->set_host(where)->start();
  sg4::ExecPtr bounded   = sg4::Exec::init()->set_flops_amount(FLOPS)->set_bound(0.8e9)->set_host(where)->start();
  double energy          = sg_host_get_consumed_energy(pm);

  sg4::this_actor::sleep_for(1);
  double u1 = unbounded->get_remaining();
  double b1 = bounded->get_remaining();
  double e1 = sg_host_get_consumed_energy(pm);

  sg4::this_actor::sleep_for(1);
  Measure full{(FLOPS - u1) / 1e9, (FLOPS - b1) / 1e9, e1 - energy};
  Measure half{(u1 - unbounded->get_remaining()) / 1e9, (b1 - bounded->get_remaining()) / 1e9,
               sg_host_get_consumed_energy(pm) - e1};
  unbounded->cancel();
  bounded->cancel();
  return {full, half};
}

static bool check(const char* what, const Measure& got, const Measure& expected)
{
  XBT_INFO("%s: tasks at %.4f and %.4f Gf/s, PM at %.4f W", what, got.unbounded, got.bounded, got.power);
  if (std::abs(got.unbounded - expected.unbounded) > 1e-6 || std::abs(got.bounded - expected.bounded) > 1e-6 ||
      std::abs(got.power - expected.power) > 1e-6) {
    XBT_INFO("FAILED: expected tasks at %.4f and %.4f Gf/s, PM at %.4f W", expected.unbounded, expected.bounded,
             expected.power);
    return false;
  }
  return true;
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg_host_energy_plugin_init();
  e.load_platform(argv[1]);
  sg4::Host* pm_alone = e.host_by_name("node-0.2cores.org");
  sg4::Host* pm_vm    = e.host_by_name("node-1.2cores.org");
  for (auto* pm : {pm_alone, pm_vm})
    pm->set_speed_profile(
        simgrid::kernel::profile::ProfileBuilder::from_string(pm->get_name() + "-half", "0 1.0\n1 0.5\n", -1));

  // Both configurations run at the same time, so that the profiles are in the same state
  e.host_by_name("node-0.1core.org")->add_actor("master", [pm_alone, pm_vm]() {
    std::pair<Measure, Measure> alone;
    std::pair<Measure, Measure> in_vm;
    auto* vm = pm_vm->create_vm("VM", 2);
    vm->start();
    sg4::ActorPtr on_pm = pm_alone->add_actor("on_pm", [&alone, pm_alone]() { alone = run_tasks(pm_alone, pm_alone); });
    sg4::ActorPtr on_vm = pm_vm->add_actor("on_vm", [&in_vm, pm_vm, vm]() { in_vm = run_tasks(pm_vm, vm); });
    on_pm->join();
    on_vm->join();
    vm->destroy();

    // At full speed, the bound limits the second task. At half speed (0.5 Gf/s per core), it does not anymore.
    // A loaded core dissipates 1W at full speed, and 0.5W at half speed on this platform.
    bool ok = check("Tasks on a PM at full speed", alone.first, {1.0, 0.8, 1.8});
    ok      = check("Tasks on a PM at half speed", alone.second, {0.5, 0.5, 1.0}) && ok;
    ok      = check("Tasks in a VM at full speed", in_vm.first, {1.0, 0.8, 1.8}) && ok;
    ok      = check("Tasks in a VM at half speed", in_vm.second, {0.5, 0.5, 1.0}) && ok;
    XBT_INFO("%s", ok ? "Passed" : "FAILED");
  });
  e.run();
  return 0;
}
