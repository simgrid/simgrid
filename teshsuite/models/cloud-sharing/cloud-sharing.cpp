/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/s4u.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this msg example");

const int FAIL_ON_ERROR = 0;
const int flop_amount   = 100000000; // 100Mf, so that computing this on a 1Gf core takes exactly 0.1s
int failed_test         = 0;

static int computation_fun(std::vector<std::string> argv)
{
  int size = std::stoi(argv[0]);

  double begin = simgrid::s4u::Engine::get_clock();
  simgrid::s4u::this_actor::execute(size);
  double end = simgrid::s4u::Engine::get_clock();

  if (0.1 - (end - begin) > 0.001) {
    xbt_assert(not FAIL_ON_ERROR, "%s with %.4g load (%dflops) took %.4fs instead of 0.1s",
               simgrid::s4u::this_actor::get_name().c_str(), ((double)size / flop_amount), size, (end - begin));
    XBT_INFO("FAILED TEST: %s with %.4g load (%dflops) took %.4fs instead of 0.1s",
             simgrid::s4u::this_actor::get_name().c_str(), ((double)size / flop_amount), size, (end - begin));
    failed_test++;
  } else {
    XBT_INFO("Passed: %s with %.4g load (%dflops) took 0.1s as expected", simgrid::s4u::this_actor::get_name().c_str(),
             ((double)size / flop_amount), size);
  }

  return 0;
}

static void run_test_process(const std::string& name, simgrid::s4u::Host* location, int size)
{
  std::vector<std::string> arg = {std::to_string(size)};
  simgrid::s4u::Actor::create(name, location, computation_fun, arg);
}

static void test_energy_consumption(const std::string& name, int nb_cores)
{
  static double current_energy = 0;
  double new_energy = 0;

  for (simgrid::s4u::Host* pm : simgrid::s4u::Engine::get_instance()->get_all_hosts()) {
    if (not dynamic_cast<simgrid::s4u::VirtualMachine*>(pm))
      new_energy += sg_host_get_consumed_energy(pm);
  }

  double expected_consumption = 0.1 * nb_cores;
  double actual_consumption   = new_energy - current_energy;

  current_energy = new_energy;

  if (std::abs(expected_consumption - actual_consumption) > 0.001) {
    XBT_INFO("FAILED TEST: %s consumed %f instead of %f J (i.e. %i cores should have been used)", name.c_str(),
             actual_consumption, expected_consumption, nb_cores);
    failed_test++;
  } else {
    XBT_INFO("Passed: %s consumed %f J (i.e. %i cores used) ", name.c_str(), actual_consumption, nb_cores);
  }
}

static void run_test(const std::string& chooser)
{
  simgrid::s4u::Host* pm0 = simgrid::s4u::Host::by_name("node-0.1core.org");
  simgrid::s4u::Host* pm1 = simgrid::s4u::Host::by_name("node-1.1core.org");
  simgrid::s4u::Host* pm2 = simgrid::s4u::Host::by_name("node-0.2cores.org"); // 2 cores
  simgrid::s4u::Host* pm4 = simgrid::s4u::Host::by_name("node-0.4cores.org");

  simgrid::s4u::VirtualMachine* vm0;
  xbt_assert(pm0, "Host node-0.1core.org does not seem to exist");
  xbt_assert(pm2, "Host node-0.2cores.org does not seem to exist");
  xbt_assert(pm4, "Host node-0.4cores.org does not seem to exist");

  // syntax of the process name:
  // "( )1" means PM with one core; "( )2" means PM with 2 cores
  // "(  [  ]2  )4" means a VM with 2 cores, on a PM with 4 cores.
  // "o" means another process is there
  // "X" means the process which holds this name

  if (chooser == "(o)1") {
    XBT_INFO("### Test '%s'. A task on a regular PM", chooser.c_str());
    run_test_process("(X)1", pm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);

  } else if (chooser == "(oo)1") {
    XBT_INFO("### Test '%s'. 2 tasks on a regular PM", chooser.c_str());
    run_test_process("(Xo)1", pm0, flop_amount / 2);
    run_test_process("(oX)1", pm0, flop_amount / 2);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);

  } else if (chooser == "(o)1 (o)1") {
    XBT_INFO("### Test '%s'. 2 regular PMs, with a task each.", chooser.c_str());
    run_test_process("(X)1 (o)1", pm0, flop_amount);
    run_test_process("(o)1 (X)1", pm1, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);

  } else if (chooser == "( [o]1 )1") {
    XBT_INFO("### Test '%s'. A task in a VM on a PM.", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm0, 1);
    run_test_process("( [X]1 )1", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [oo]1 )1") {
    XBT_INFO("### Test '%s'. 2 tasks co-located in a VM on a PM.", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm0, 1);
    run_test_process("( [Xo]1 )1", vm0, flop_amount / 2);
    run_test_process("( [oX]1 )1", vm0, flop_amount / 2);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [ ]1 o )1") {
    XBT_INFO("### Test '%s'. 1 task collocated with an empty VM", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm0, 1);
    run_test_process("( [ ]1 X )1", pm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [o]1 o )1") {
    XBT_INFO("### Test '%s'. A task in a VM, plus a task", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm0, 1);
    run_test_process("( [X]1 o )1", vm0, flop_amount / 2);
    run_test_process("( [o]1 X )1", pm0, flop_amount / 2);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [oo]1 o )1") {
    XBT_INFO("### Test '%s'. 2 tasks in a VM, plus a task", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm0, 1);
    run_test_process("( [Xo]1 o )1", vm0, flop_amount / 4);
    run_test_process("( [oX]1 o )1", vm0, flop_amount / 4);
    run_test_process("( [oo]1 X )1", pm0, flop_amount / 2);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( o )2") {
    XBT_INFO("### Test '%s'. A task on bicore PM", chooser.c_str());
    run_test_process("(X)2", pm2, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);

  } else if (chooser == "( oo )2") {
    XBT_INFO("### Test '%s'. 2 tasks on a bicore PM", chooser.c_str());
    run_test_process("(Xx)2", pm2, flop_amount);
    run_test_process("(xX)2", pm2, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);

  } else if (chooser == "( ooo )2") {
    XBT_INFO("### Test '%s'. 3 tasks on a bicore PM", chooser.c_str());
    run_test_process("(Xxx)2", pm2, flop_amount * 2 / 3);
    run_test_process("(xXx)2", pm2, flop_amount * 2 / 3);
    run_test_process("(xxX)2", pm2, flop_amount * 2 / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);

  } else if (chooser == "( [o]1 )2") {
    XBT_INFO("### Test '%s'. A task in a VM on a bicore PM", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    run_test_process("( [X]1 )2", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [oo]1 )2") {
    XBT_INFO("### Test '%s'. 2 tasks in a VM on a bicore PM", chooser.c_str());
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    run_test_process("( [Xx]1 )2", vm0, flop_amount / 2);
    run_test_process("( [xX]1 )2", vm0, flop_amount / 2);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [ ]1 o )2") {
    XBT_INFO("### Put a VM on a PM, and put a task to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    run_test_process("( [ ]1 X )2", pm2, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [o]1 o )2") {
    XBT_INFO("### Put a VM on a PM, put a task to the PM and a task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    run_test_process("( [X]1 x )2", vm0, flop_amount);
    run_test_process("( [x]1 X )2", pm2, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [o]1 [ ]1 )2") {
    XBT_INFO("### Put two VMs on a PM, and put a task to one VM");
    vm0       = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    auto* vm1 = new simgrid::s4u::VirtualMachine("VM1", pm2, 1);
    run_test_process("( [X]1 [ ]1 )2", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();
    vm1->destroy();

  } else if (chooser == "( [o]1 [o]1 )2") {
    XBT_INFO("### Put two VMs on a PM, and put a task to each VM");
    vm0       = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    auto* vm1 = new simgrid::s4u::VirtualMachine("VM1", pm2, 1);
    run_test_process("( [X]1 [x]1 )2", vm0, flop_amount);
    run_test_process("( [x]1 [X]1 )2", vm1, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();
    vm1->destroy();

  } else if (chooser == "( [o]1 [o]1 [ ]1 )2") {
    XBT_INFO("### Put three VMs on a PM, and put a task to two VMs");
    vm0       = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    auto* vm1 = new simgrid::s4u::VirtualMachine("VM1", pm2, 1);
    auto* vm2 = new simgrid::s4u::VirtualMachine("VM2", pm2, 1);
    run_test_process("( [X]1 [x]1 [ ]1 )2", vm0, flop_amount);
    run_test_process("( [x]1 [X]1 [ ]1 )2", vm1, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();
    vm1->destroy();
    vm2->destroy();

  } else if (chooser == "( [o]1 [o]1 [o]1 )2") {
    XBT_INFO("### Put three VMs on a PM, and put a task to each VM");
    vm0       = new simgrid::s4u::VirtualMachine("VM0", pm2, 1);
    auto* vm1 = new simgrid::s4u::VirtualMachine("VM1", pm2, 1);
    auto* vm2 = new simgrid::s4u::VirtualMachine("VM2", pm2, 1);
    run_test_process("( [X]1 [o]1 [o]1 )2", vm0, flop_amount * 2 / 3);
    run_test_process("( [o]1 [X]1 [o]1 )2", vm1, flop_amount * 2 / 3);
    run_test_process("( [o]1 [o]1 [X]1 )2", vm2, flop_amount * 2 / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();
    vm1->destroy();
    vm2->destroy();

  } else if (chooser == "( [o]2 )2") {
    XBT_INFO("### Put a VM on a PM, and put a task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [X]2 )2", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [oo]2 )2") {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [Xo]2 )2", vm0, flop_amount);
    run_test_process("( [oX]2 )2", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ooo]2 )2") {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [Xoo]2 )2", vm0, flop_amount * 2 / 3);
    run_test_process("( [oXo]2 )2", vm0, flop_amount * 2 / 3);
    run_test_process("( [ooX]2 )2", vm0, flop_amount * 2 / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ ]2 o )2") {
    XBT_INFO("### Put a VM on a PM, and put a task to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [ ]2 X )2", pm2, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [o]2 o )2") {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [o]2 X )2", pm2, flop_amount);
    run_test_process("( [X]2 o )2", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [oo]2 o )2") {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [oo]2 X )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [Xo]2 o )2", vm0, flop_amount * 2 / 3);
    run_test_process("( [oX]2 o )2", vm0, flop_amount * 2 / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ooo]2 o )2") {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [ooo]2 X )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [Xoo]2 o )2", vm0, (flop_amount * 4 / 3) / 3); // VM_share/3
    run_test_process("( [oXo]2 o )2", vm0, (flop_amount * 4 / 3) / 3); // VM_share/3
    run_test_process("( [ooX]2 o )2", vm0, (flop_amount * 4 / 3) / 3); // VM_share/3
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ ]2 oo )2") {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [ ]2 Xo )2", pm2, flop_amount);
    run_test_process("( [ ]2 oX )2", pm2, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [o]2 oo )2") {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and one task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [o]2 Xo )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [o]2 oX )2", pm2, flop_amount * 2 / 3);
    run_test_process("( [X]2 oo )2", vm0, flop_amount * 2 / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [oo]2 oo )2") {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and two tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [oo]2 Xo )2", pm2, flop_amount / 2);
    run_test_process("( [oo]2 oX )2", pm2, flop_amount / 2);
    run_test_process("( [Xo]2 oo )2", vm0, flop_amount / 2);
    run_test_process("( [oX]2 oo )2", vm0, flop_amount / 2);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ooo]2 oo )2") {
    XBT_INFO("### Put a VM on a PM, put one task to the PM and three tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm2, 2);
    run_test_process("( [ooo]2 Xo )2", pm2, flop_amount * 2 / 4);
    run_test_process("( [ooo]2 oX )2", pm2, flop_amount * 2 / 4);
    run_test_process("( [Xoo]2 oo )2", vm0, flop_amount / 3);
    run_test_process("( [oXo]2 oo )2", vm0, flop_amount / 3);
    run_test_process("( [ooX]2 oo )2", vm0, flop_amount / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [o]2 )4") {
    XBT_INFO("### Put a VM on a PM, and put a task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [X]2 )4", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [oo]2 )4") {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [Xo]2 )4", vm0, flop_amount);
    run_test_process("( [oX]2 )4", vm0, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ooo]2 )4") {
    XBT_INFO("### ( [ooo]2 )4: Put a VM on a PM, and put three tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [Xoo]2 )4", vm0, flop_amount * 2 / 3);
    run_test_process("( [oXo]2 )4", vm0, flop_amount * 2 / 3);
    run_test_process("( [ooX]2 )4", vm0, flop_amount * 2 / 3);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ ]2 o )4") {
    XBT_INFO("### Put a VM on a PM, and put a task to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [ ]2 X )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 1);
    vm0->destroy();

  } else if (chooser == "( [ ]2 oo )4") {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [ ]2 Xo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oX )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [ ]2 ooo )4") {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [ ]2 Xoo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oXo )4", pm4, flop_amount);
    run_test_process("( [ ]2 ooX )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 3);
    vm0->destroy();

  } else if (chooser == "( [ ]2 oooo )4") {
    XBT_INFO("### Put a VM on a PM, and put four tasks to the PM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [ ]2 Xooo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oXoo )4", pm4, flop_amount);
    run_test_process("( [ ]2 ooXo )4", pm4, flop_amount);
    run_test_process("( [ ]2 oooX )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 4);
    vm0->destroy();

  } else if (chooser == "( [o]2 o )4") {
    XBT_INFO("### Put a VM on a PM, and put one task to the PM and one task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [X]2 o )4", vm0, flop_amount);
    run_test_process("( [o]2 X )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 2);
    vm0->destroy();

  } else if (chooser == "( [o]2 oo )4") {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and one task to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [X]2 oo )4", vm0, flop_amount);
    run_test_process("( [o]2 Xo )4", pm4, flop_amount);
    run_test_process("( [o]2 oX )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 3);
    vm0->destroy();

  } else if (chooser == "( [oo]2 oo )4") {
    XBT_INFO("### Put a VM on a PM, and put two tasks to the PM and two tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [Xo]2 oo )4", vm0, flop_amount);
    run_test_process("( [oX]2 oo )4", vm0, flop_amount);
    run_test_process("( [oo]2 Xo )4", pm4, flop_amount);
    run_test_process("( [oo]2 oX )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 4);
    vm0->destroy();

  } else if (chooser == "( [o]2 ooo )4") {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and one tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [X]2 ooo )4", vm0, flop_amount);
    run_test_process("( [o]2 Xoo )4", pm4, flop_amount);
    run_test_process("( [o]2 oXo )4", pm4, flop_amount);
    run_test_process("( [o]2 ooX )4", pm4, flop_amount);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 4);
    vm0->destroy();

  } else if (chooser == "( [oo]2 ooo )4") {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and two tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [Xo]2 ooo )4", vm0, flop_amount * 4 / 5);
    run_test_process("( [oX]2 ooo )4", vm0, flop_amount * 4 / 5);
    run_test_process("( [oo]2 Xoo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [oo]2 oXo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [oo]2 ooX )4", pm4, flop_amount * 4 / 5);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 4);
    vm0->destroy();

  } else if (chooser == "( [ooo]2 ooo )4") {
    XBT_INFO("### Put a VM on a PM, and put three tasks to the PM and three tasks to the VM");
    vm0 = new simgrid::s4u::VirtualMachine("VM0", pm4, 2);
    run_test_process("( [Xoo]2 ooo )4", vm0, (flop_amount * 8 / 5) / 3); // The VM has 8/5 of the PM
    run_test_process("( [oXo]2 ooo )4", vm0, (flop_amount * 8 / 5) / 3);
    run_test_process("( [ooX]2 ooo )4", vm0, (flop_amount * 8 / 5) / 3);

    run_test_process("( [ooo]2 Xoo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [ooo]2 oXo )4", pm4, flop_amount * 4 / 5);
    run_test_process("( [ooo]2 ooX )4", pm4, flop_amount * 4 / 5);
    simgrid::s4u::this_actor::sleep_for(2);
    test_energy_consumption(chooser, 4);
    vm0->destroy();

  } else {
    xbt_die("Unknown chooser: %s", chooser.c_str());
  }
}
static int master_main()
{
  XBT_INFO("# TEST ON SINGLE-CORE PMs");
  XBT_INFO("## Check computation on regular PMs");
  run_test("(o)1");
  run_test("(oo)1");
  run_test("(o)1 (o)1");
  XBT_INFO("# TEST ON SINGLE-CORE PMs AND SINGLE-CORE VMs");

  XBT_INFO("## Check the impact of running tasks inside a VM (no degradation for the moment)");
  run_test("( [o]1 )1");
  run_test("( [oo]1 )1");

  XBT_INFO("## Check impact of running tasks collocated with VMs (no VM noise for the moment)");
  run_test("( [ ]1 o )1");
  run_test("( [o]1 o )1");
  run_test("( [oo]1 o )1");

  XBT_INFO("# TEST ON TWO-CORE PMs");
  XBT_INFO("## Check computation on 2 cores PMs");
  run_test("( o )2");
  run_test("( oo )2");
  run_test("( ooo )2");

  XBT_INFO("# TEST ON TWO-CORE PMs AND SINGLE-CORE VMs");
  XBT_INFO("## Check impact of a single VM (no degradation for the moment)");
  run_test("( [o]1 )2");
  run_test("( [oo]1 )2");
  run_test("( [ ]1 o )2");
  run_test("( [o]1 o )2");

  XBT_INFO("## Check impact of a several VMs (there is no degradation for the moment)");
  run_test("( [o]1 [ ]1 )2");
  run_test("( [o]1 [o]1 )2");
  run_test("( [o]1 [o]1 [ ]1 )2");
  run_test("( [o]1 [o]1 [o]1 )2");

  XBT_INFO("# TEST ON TWO-CORE PMs AND TWO-CORE VMs");

  XBT_INFO("## Check impact of a single VM (there is no degradation for the moment)");
  run_test("( [o]2 )2");
  run_test("( [oo]2 )2");
  run_test("( [ooo]2 )2");

  XBT_INFO("## Check impact of a single VM collocated with a task (there is no degradation for the moment)");
  run_test("( [ ]2 o )2");
  run_test("( [o]2 o )2");
  run_test("( [oo]2 o )2");
  run_test("( [ooo]2 o )2");
  run_test("( [ ]2 oo )2");
  run_test("( [o]2 oo )2");
  run_test("( [oo]2 oo )2");
  run_test("( [ooo]2 oo )2");

  XBT_INFO("# TEST ON FOUR-CORE PMs AND TWO-CORE VMs");

  XBT_INFO("## Check impact of a single VM");
  run_test("( [o]2 )4");
  run_test("( [oo]2 )4");
  run_test("( [ooo]2 )4");

  XBT_INFO("## Check impact of a single empty VM collocated with tasks");
  run_test("( [ ]2 o )4");
  run_test("( [ ]2 oo )4");
  run_test("( [ ]2 ooo )4");
  run_test("( [ ]2 oooo )4");

  XBT_INFO("## Check impact of a single working VM collocated with tasks");
  run_test("( [o]2 o )4");
  run_test("( [o]2 oo )4");
  run_test("( [oo]2 oo )4");
  run_test("( [o]2 ooo )4");
  run_test("( [oo]2 ooo )4");
  run_test("( [ooo]2 ooo )4");

  XBT_INFO("   ");
  XBT_INFO("   ");
  XBT_INFO("## %d test failed", failed_test);
  XBT_INFO("   ");
  return 0;
}
int main(int argc, char* argv[])
{
  /* Get the arguments */
  simgrid::s4u::Engine e(&argc, argv);
  sg_host_energy_plugin_init();

  /* load the platform file */
  const char* platform = "../../../platforms/cloud-sharing.xml";
  if (argc == 2)
    platform = argv[1];
  e.load_platform(platform);

  simgrid::s4u::Host* pm0 = e.host_by_name("node-0.1core.org");
  xbt_assert(pm0, "Host 'node-0.1core.org' not found");
  simgrid::s4u::Actor::create("master", pm0, master_main);

  e.run();

  return failed_test;
}
