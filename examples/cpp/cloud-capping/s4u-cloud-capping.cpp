/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/live_migration.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void worker(double computation_amount, bool use_bound, double bound)
{
  double clock_start = sg4::Engine::get_clock();

  sg4::ExecPtr exec = sg4::this_actor::exec_init(computation_amount);

  if (use_bound) {
    if (bound < 1e-12) /* close enough to 0 without any floating precision surprise */
      XBT_INFO("bound == 0 means no capping (i.e., unlimited).");
    exec->set_bound(bound);
  }
  exec->start();
  exec->wait();
  double clock_end     = sg4::Engine::get_clock();
  double duration      = clock_end - clock_start;
  double flops_per_sec = computation_amount / duration;

  if (use_bound)
    XBT_INFO("bound to %f => duration %f (%f flops/s)", bound, duration, flops_per_sec);
  else
    XBT_INFO("not bound => duration %f (%f flops/s)", duration, flops_per_sec);
}

static void worker_busy_loop(const char* name, double speed)
{
  double exec_remain_prev    = 1e11;
  sg4::ExecPtr exec          = sg4::this_actor::exec_async(exec_remain_prev);
  for (int i = 0; i < 10; i++) {
    if (speed > 0) {
      double new_bound = (speed / 10) * i;
      XBT_INFO("set bound of VM1 to %f", new_bound);
      static_cast<sg4::VirtualMachine*>(sg4::this_actor::get_host())->set_bound(new_bound);
    }
    sg4::this_actor::sleep_for(100);
    double exec_remain_now = exec->get_remaining();
    double flops_per_sec   = exec_remain_prev - exec_remain_now;
    XBT_INFO("%s@%s: %.0f flops/s", name, sg4::this_actor::get_host()->get_cname(), flops_per_sec / 100);
    exec_remain_prev = exec_remain_now;
    sg4::this_actor::sleep_for(1);
  }
  exec->wait();
}

static void test_dynamic_change()
{
  sg4::Host* pm0 = sg4::Host::by_name("Fafard");

  auto* vm0 = pm0->create_vm("VM0", 1);
  auto* vm1 = pm0->create_vm("VM1", 1);
  vm0->start();
  vm1->start();

  vm0->add_actor("worker0", worker_busy_loop, "Task0", -1);
  vm1->add_actor("worker1", worker_busy_loop, "Task1", pm0->get_speed());

  sg4::this_actor::sleep_for(3000); // let the activities end
  vm0->destroy();
  vm1->destroy();
}

static void test_one_activity(sg4::Engine& e, sg4::Host* host)
{
  const double cpu_speed          = host->get_speed();
  const double computation_amount = cpu_speed * 10;

  XBT_INFO("### Test: with/without activity set_bound");

  XBT_INFO("### Test: no bound for Task1@%s", host->get_cname());
  host->add_actor("worker0", worker, computation_amount, false, 0);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 50%% for Task1@%s", host->get_cname());
  host->add_actor("worker0", worker, computation_amount, true, cpu_speed / 2);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 33%% for Task1@%s", host->get_cname());
  host->add_actor("worker0", worker, computation_amount, true, cpu_speed / 3);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: zero for Task1@%s (i.e., unlimited)", host->get_cname());
  host->add_actor("worker0", worker, computation_amount, true, 0);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 200%% for Task1@%s (i.e., meaningless)", host->get_cname());
  host->add_actor("worker0", worker, computation_amount, true, cpu_speed * 2);

  sg4::this_actor::sleep_for(1000);
}

static void test_two_activities(sg4::Engine& e, sg4::Host* hostA, sg4::Host* hostB)
{
  const double cpu_speed = hostA->get_speed();
  xbt_assert(cpu_speed == hostB->get_speed());
  const double computation_amount = cpu_speed * 10;
  const char* hostA_name          = hostA->get_cname();
  const char* hostB_name          = hostB->get_cname();

  XBT_INFO("### Test: no bound for Task1@%s, no bound for Task2@%s", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, false, 0);
  hostB->add_actor("worker1", worker, computation_amount, false, 0);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 0 for Task1@%s, 0 for Task2@%s (i.e., unlimited)", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, true, 0);
  hostB->add_actor("worker1", worker, computation_amount, true, 0);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 50%% for Task1@%s, 50%% for Task2@%s", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, true, cpu_speed / 2);
  hostB->add_actor("worker1", worker, computation_amount, true, cpu_speed / 2);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 25%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, true, cpu_speed / 4);
  hostB->add_actor("worker1", worker, computation_amount, true, cpu_speed / 4);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 75%% for Task1@%s, 100%% for Task2@%s", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, true, cpu_speed * 0.75);
  hostB->add_actor("worker1", worker, computation_amount, true, cpu_speed);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: no bound for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, false, 0);
  hostB->add_actor("worker1", worker, computation_amount, true, cpu_speed / 4);

  sg4::this_actor::sleep_for(1000);

  XBT_INFO("### Test: 75%% for Task1@%s, 25%% for Task2@%s", hostA_name, hostB_name);
  hostA->add_actor("worker0", worker, computation_amount, true, cpu_speed * 0.75);
  hostB->add_actor("worker1", worker, computation_amount, true, cpu_speed / 4);

  sg4::this_actor::sleep_for(1000);
}

static void master_main()
{
  sg4::Engine& e = *sg4::this_actor::get_engine();
  sg4::Host* pm0 = sg4::Host::by_name("Fafard");

  XBT_INFO("# 1. Put a single activity on a PM.");
  test_one_activity(e, pm0);
  XBT_INFO(".");

  XBT_INFO("# 2. Put two activities on a PM.");
  test_two_activities(e, pm0, pm0);
  XBT_INFO(".");

  auto* vm0 = pm0->create_vm("VM0", 1);
  vm0->start();

  XBT_INFO("# 3. Put a single activity on a VM.");
  test_one_activity(e, vm0);
  XBT_INFO(".");

  XBT_INFO("# 4. Put two activities on a VM.");
  test_two_activities(e, vm0, vm0);
  XBT_INFO(".");

  vm0->destroy();

  vm0 = pm0->create_vm("VM0", 1);
  vm0->start();

  XBT_INFO("# 6. Put an activity on a PM and an activity on a VM.");
  test_two_activities(e, pm0, vm0);
  XBT_INFO(".");

  vm0->destroy();

  vm0 = pm0->create_vm("VM0", 1);
  vm0->set_bound(pm0->get_speed() / 10);
  vm0->start();

  XBT_INFO("# 7. Put a single activity on the VM capped by 10%%.");
  test_one_activity(e, vm0);
  XBT_INFO(".");

  XBT_INFO("# 8. Put two activities on the VM capped by 10%%.");
  test_two_activities(e, vm0, vm0);
  XBT_INFO(".");

  XBT_INFO("# 9. Put an activity on a PM and an activity on the VM capped by 10%%.");
  test_two_activities(e, pm0, vm0);
  XBT_INFO(".");

  vm0->destroy();

  vm0 = pm0->create_vm("VM0", 1);
  vm0->set_ramsize(1e9); // 1GB
  vm0->start();

  double cpu_speed = pm0->get_speed();

  XBT_INFO("# 10. Test migration");
  const double computation_amount = cpu_speed * 10;

  XBT_INFO("# 10. (a) Put an activity on a VM without any bound.");
  vm0->add_actor("worker0", worker, computation_amount, false, 0);
  sg4::this_actor::sleep_for(1000);
  XBT_INFO(".");

  XBT_INFO("# 10. (b) set 10%% bound to the VM, and then put an activity on the VM.");
  vm0->set_bound(cpu_speed / 10);
  vm0->add_actor("worker0", worker, computation_amount, false, 0);
  sg4::this_actor::sleep_for(1000);
  XBT_INFO(".");

  XBT_INFO("# 10. (c) migrate");
  sg4::Host* pm1 = sg4::Host::by_name("Fafard");
  sg_vm_migrate(vm0, pm1);
  XBT_INFO(".");

  XBT_INFO("# 10. (d) Put an activity again on the VM.");
  vm0->add_actor("worker0", worker, computation_amount, false, 0);
  sg4::this_actor::sleep_for(1000);
  XBT_INFO(".");

  vm0->destroy();

  XBT_INFO("# 11. Change a bound dynamically.");
  test_dynamic_change();
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg_vm_live_migration_plugin_init();
  /* load the platform file */
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  e.host_by_name("Fafard")->add_actor("master_", master_main);

  e.run();
  XBT_INFO("Bye (simulation time %g)", e.get_clock());

  return 0;
}
