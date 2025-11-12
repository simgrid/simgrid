/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this example");

static void computation_fun(sg4::ExecPtr& exec)
{
  const char* pr_name   = sg4::this_actor::get_cname();
  const char* host_name = sg4::Host::current()->get_cname();
  double clock_sta      = sg4::Engine::get_clock();

  XBT_INFO("%s:%s Exec 1 start %g", host_name, pr_name, clock_sta);
  exec = sg4::this_actor::exec_async(1e9);
  exec->wait();
  XBT_INFO("%s:%s Exec 1 complete %g", host_name, pr_name, sg4::Engine::get_clock() - clock_sta);

  exec = nullptr;

  sg4::this_actor::sleep_for(1);

  clock_sta = sg4::Engine::get_clock();
  XBT_INFO("%s:%s Exec 2 start %g", host_name, pr_name, clock_sta);
  exec = sg4::this_actor::exec_async(1e10);
  exec->wait();
  XBT_INFO("%s:%s Exec 2 complete %g", host_name, pr_name, sg4::Engine::get_clock() - clock_sta);
}

static void master_main()
{
  auto* pm0 = sg4::Host::by_name("Fafard");
  auto* vm0 = pm0->create_vm("VM0", 1);
  vm0->start();

  sg4::ExecPtr exec;
  vm0->add_actor("compute",computation_fun, std::ref(exec));

  while (sg4::Engine::get_clock() < 100) {
    if (exec)
      XBT_INFO("exec remaining duration: %g", exec->get_remaining());
    sg4::this_actor::sleep_for(1);
  }

  sg4::this_actor::sleep_for(10000);
  vm0->destroy();
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  e.host_by_name("Fafard")->add_actor("master_", master_main);

  e.run();
  XBT_INFO("Bye (simulation time %g)", e.get_clock());

  return 0;
}
