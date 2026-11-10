/* Copyright (c) 2026. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Check that a VM stops loading its PM when its execs terminate, even when nobody waits for them.
 *
 * The VM model counts the execs running in each VM to decide how many cores of the PM the VM uses. An exec must leave
 * this count when it terminates, whatever the reason: when its actor is killed, when its actor terminates without
 * waiting for it, or when it completes while nobody waits for it. Otherwise, the VM keeps loading its PM forever.
 *
 * On the hosts used here, a core computes 1Gf/s and consumes 1W when loaded and 0W when idle. Each scenario runs an
 * exec of 0.2 Gf in a VM, and measures the energy dissipated by the PM over 1.2s. */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include <cmath>
#include <functional>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this test");

namespace sg4 = simgrid::s4u;

constexpr double FLOP_AMOUNT = 200e6; // 0.2s on a 1Gf core
constexpr double PERIOD      = 1.2;
int failed_test              = 0;

static double total_energy()
{
  double energy = 0;
  for (sg4::Host* host : sg4::Engine::get_instance()->get_all_hosts())
    if (dynamic_cast<sg4::VirtualMachine*>(host) == nullptr)
      energy += sg_host_get_consumed_energy(host);
  return energy;
}

/* Run the 'worker' function in a fresh VM for PERIOD seconds, and check the energy dissipated meanwhile.
 * If 'kill_after' is positive, the worker gets killed after that delay. */
static void run_scenario(const std::string& name, const std::function<void()>& worker, double expected_energy,
                         double kill_after = -1)
{
  sg4::Host* pm       = sg4::Host::by_name("node-0.2cores.org");
  auto* vm            = pm->create_vm("VM0", 2);
  double start_energy = total_energy();

  sg4::ActorPtr actor = vm->add_actor("worker", worker);
  if (kill_after > 0) {
    sg4::this_actor::sleep_for(kill_after);
    actor->kill();
    sg4::this_actor::sleep_for(PERIOD - kill_after);
  } else {
    sg4::this_actor::sleep_for(PERIOD);
  }

  double consumed = total_energy() - start_energy;
  if (std::abs(consumed - expected_energy) > 0.001) {
    XBT_INFO("FAILED TEST: %s: consumed %.4fJ instead of %.4fJ", name.c_str(), consumed, expected_energy);
    failed_test++;
  } else {
    XBT_INFO("Passed: %s", name.c_str());
  }
  vm->destroy();
}

static void master_main()
{
  XBT_INFO("### An actor gets killed while computing in a VM");
  run_scenario("Killed actor", [] { sg4::this_actor::execute(FLOP_AMOUNT); }, 0.1, 0.1);

  XBT_INFO("### An actor terminates without waiting for its exec");
  run_scenario(
      "Terminated actor",
      [] {
        sg4::this_actor::exec_async(FLOP_AMOUNT);
        sg4::this_actor::sleep_for(0.1);
      },
      0.1);

  XBT_INFO("### An exec completes while nobody waits for it");
  run_scenario(
      "Exec never waited",
      [] {
        sg4::this_actor::exec_async(FLOP_AMOUNT);
        sg4::this_actor::sleep_for(PERIOD);
      },
      0.2);

  XBT_INFO("### An exec gets canceled by its owner");
  run_scenario(
      "Canceled exec",
      [] {
        sg4::ExecPtr exec = sg4::this_actor::exec_async(FLOP_AMOUNT);
        sg4::this_actor::sleep_for(0.1);
        exec->cancel();
      },
      0.1);

  XBT_INFO("### An exec completes normally (control case)");
  run_scenario("Waited exec", [] { sg4::this_actor::execute(FLOP_AMOUNT); }, 0.2);

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
