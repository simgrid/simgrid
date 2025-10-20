/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/load.h"
#include "simgrid/s4u.hpp"
#include <list>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void execute_load_test()
{
  std::list<simgrid::s4u::VirtualMachine *> vms;

  s4u_Host* host = sg4::Host::by_name("MyHost1");

  simgrid::s4u::VirtualMachine *vm_1 = host->create_vm("VM_1", 1);
  vms.push_back(vm_1);

  simgrid::s4u::VirtualMachine *vm_2 = host->create_vm("VM_2", 1);
  vms.push_back(vm_2);

  s4u_Host* host_2 = sg4::Host::by_name("MyHost2");

  simgrid::s4u::VirtualMachine *vm_3 = host_2->create_vm("VM_3", 2);
  vms.push_back(vm_3);

  for (auto const& vm : vms)
    XBT_INFO("Initial. VM %s on machine %s using %d cores. Peak speed: %.0e Computed flops: %.0e (should be 0) Average Load: %.0e (should be 0)",
      vm->get_cname(),
      vm->get_pm()->get_cname(),
      vm->get_core_count(),
      vm->get_pm()->get_speed(),
      sg_vm_get_computed_flops(vm),
      sg_vm_get_avg_load(vm)
    );

  XBT_INFO("Sleep for 10 seconds");
  sg4::this_actor::sleep_for(10);

  double host_1_speed = vm_1->get_pm()->get_speed();
  double host_2_speed = vm_3->get_pm()->get_speed();

  for (auto const& vm : vms)
    XBT_INFO("Done sleeping. %s Peak speed: %.1e Computed flops: %.0e (should be 0) Average Load: %.0e (should be 0)",
      vm->get_cname(),
      vm->get_pm()->get_speed(),
      sg_vm_get_computed_flops(vm),
      sg_vm_get_avg_load(vm)
    );

  // Run an activity
  XBT_INFO("========= Starting activities on VM_1 and VM_2");
  double start = sg4::Engine::get_clock();
  s4u_ActivitySet act;

  double flops_1 = 2e8;
  double flops_2 = 1.5e8;

  XBT_INFO("Run an activity of %.1e flops on VM_1", flops_1);
  XBT_INFO("Run an activity of %.1e flops on VM_2", flops_2);
  
  auto vm_exec_1 = vm_1->exec_init(flops_1);
  // We need to set again the host, because exec_init takes the physical machine instead of the VM
  vm_exec_1->set_host(vm_1);
  act.push(vm_exec_1);
  auto vm_exec_2 = vm_2->exec_init(flops_2);
  // We need to set again the host, because exec_init takes the physical machine instead of the VM
  vm_exec_2->set_host(vm_2);
  act.push(vm_exec_2);
  
  vm_exec_1->start();
  vm_exec_2->start();
  double time_1 = -1;
  double time_2 = -1;

  while (!act.empty()) {
    auto pointer = act.wait_any();
    if (pointer == vm_exec_1)
      time_1 = sg4::Engine::get_clock();
    else
      time_2 = sg4::Engine::get_clock();
  }

  XBT_INFO("Done activity on %s. "
          "It took %.2fs (should be %.2fs). "
          "Peak speed: %.1e (should be 2E+07) "
          "Computed flops: %.1e (should be %.1e) "
          "Average Load: %.8f (should be %.8f) ",
    vm_1->get_cname(),
    time_1 - start,
    ((flops_1 - (0.5 * host_1_speed)) / vm_1->get_pm()->get_speed()) + 0.5,
    vm_1->get_pm()->get_speed(),
    sg_vm_get_computed_flops(vm_1),
    flops_1,
    sg_vm_get_avg_load(vm_1),
    flops_1 / (10.5 * host_1_speed * vm_1->get_core_count() +
               (sg4::Engine::get_clock() - start - 0.5) * vm_1->get_pm()->get_speed() * vm_1->get_core_count())
  );

  XBT_INFO("Done activity on %s. "
          "It took %.2fs (should be %.2fs). "
          "Peak speed: %.1e (should be 2E+07) "
          "Computed flops: %.1e (should be %.1e) "
          "Average Load: %.8f (should be %.8f) ",
    vm_2->get_cname(),
    time_2 - start,
    ((flops_2 - (0.5 * host_1_speed)) / vm_2->get_pm()->get_speed()) + 0.5,
    vm_2->get_pm()->get_speed(),
    sg_vm_get_computed_flops(vm_2),
    flops_2,
    sg_vm_get_avg_load(vm_2),
    flops_2 / (10.5 * host_1_speed * vm_2->get_core_count() +
               (sg4::Engine::get_clock() - start - 0.5) * vm_2->get_pm()->get_speed() * vm_2->get_core_count())
  );

  XBT_INFO("%s was idle (so, no computation). "
          "Peak speed: %.1e (should be 1E+08) "
          "Computed flops: %.1e (should be %.1e) "
          "Average Load: %.8f (should be %.8f) ",
    vm_3->get_cname(),
    vm_3->get_pm()->get_speed(),
    sg_vm_get_computed_flops(vm_3),
    0.0,
    sg_vm_get_avg_load(vm_3),
    0.0
  );   

  // ========= Change power peak =========
  int pstate = 1;
  host_2->set_pstate(pstate);
  XBT_INFO(
      "========= Requesting pstate %d on machine %s", pstate, host_2->get_cname());
  for (auto const& vm : vms)
    XBT_INFO("Machine %s Peak speed: %.1e",
      vm->get_cname(),
      vm->get_pm()->get_speed()
    );      
  
  XBT_INFO("========= Starting activities on VM_3");
  // Run a second activity
  double start_2 = sg4::Engine::get_clock();
  double flops_3 = 1E8;
  XBT_INFO("Run an activity of %.1e flops on VM_3", flops_3);
  vm_exec_1 = vm_3->exec_init(flops_3);
  // We need to set again the host, because exec_init takes the physical machine instead of the VM
  vm_exec_1->set_host(vm_3);
  vm_exec_1->start();
  vm_exec_1->wait();


  XBT_INFO("%s was idle (so, no computation). "
          "Peak speed: %.1e (should be 2E+07) "
          "Computed flops: %.1e (should be %.1e) "
          "Average Load: %.8f (should be %.8f) ",
    vm_1->get_cname(),
    vm_1->get_pm()->get_speed(),
    sg_vm_get_computed_flops(vm_1),
    flops_1,
    sg_vm_get_avg_load(vm_1),
    flops_1 / (10.5 * host_1_speed * vm_1->get_core_count() +
               (sg4::Engine::get_clock() - start - 0.5) * vm_1->get_pm()->get_speed() * vm_1->get_core_count())
  );

  XBT_INFO("%s was idle (so, no computation). "
          "Peak speed: %.1e (should be 2E+07) "
          "Computed flops: %.1e (should be %.1e) "
          "Average Load: %.8f (should be %.8f) ",
    vm_2->get_cname(),
    vm_2->get_pm()->get_speed(),
    sg_vm_get_computed_flops(vm_2),
    flops_2,
    sg_vm_get_avg_load(vm_2),
    flops_2 / (10.5 * host_1_speed * vm_2->get_core_count() +
               (sg4::Engine::get_clock() - start - 0.5) * vm_2->get_pm()->get_speed() * vm_2->get_core_count())
  );  

  XBT_INFO("Done activity on %s. "
          "It took %.2fs (should be %.2fs). "
          "Peak speed: %.1e (should be 5E+07) "
          "Computed flops: %.1e (should be %.1e) "
          "Average Load: %.8f (should be %.8f) ",
    vm_3->get_cname(),
    sg4::Engine::get_clock() - start_2,
    flops_3 / vm_3->get_pm()->get_speed(),
    vm_3->get_pm()->get_speed(),
    sg_vm_get_computed_flops(vm_3),
    flops_3,
    sg_vm_get_avg_load(vm_3),
    flops_3 / (start_2 * host_2_speed * vm_3->get_core_count() + (sg4::Engine::get_clock() - start_2) * vm_3->get_pm()->get_speed() * vm_3->get_core_count())
  );

  XBT_INFO("========= Requesting a reset of the computation and load counters");
  sg_vm_load_reset(vm_1);
  sg_vm_load_reset(vm_2);
  sg_vm_load_reset(vm_3);
  for (auto const& vm : vms)
    XBT_INFO("After reseting. %s Peak speed: %.1e Computed flops: %.1e (should be 0) Average Load: %.1e (should be 0)",
      vm->get_cname(),
      vm->get_pm()->get_speed(),
      sg_vm_get_computed_flops(vm),
      sg_vm_get_avg_load(vm)
    );
  XBT_INFO("Sleep for 4 seconds");
  sg4::this_actor::sleep_for(4);
  for (auto const& vm : vms)
    XBT_INFO("Done sleeping. %s Peak speed: %.1e Computed flops: %.1e (should be 0) Average Load: %.1e (should be 0)",
      vm->get_cname(),
      vm->get_pm()->get_speed(),
      sg_vm_get_computed_flops(vm),
      sg_vm_get_avg_load(vm)
    );

}

static void change_speed()
{
  s4u_Host* host = sg4::Host::by_name("MyHost1");
  sg4::this_actor::sleep_for(10.5);
  XBT_INFO("I slept until now, but now I'll change the speed of this host "
           "while the other actor is still computing! This should slow the computation down.");
  host->set_pstate(2);
}

int main(int argc, char* argv[])
{
  sg_vm_load_plugin_init();
  sg4::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);
  e.load_platform(argv[1]);

  auto* host = e.host_by_name("MyHost1");
  host->add_actor("load_test", execute_load_test);
  host->add_actor("change_speed", change_speed);

  e.run();

  XBT_INFO("Total simulation time: %.2f", sg4::Engine::get_clock());

  return 0;
}
