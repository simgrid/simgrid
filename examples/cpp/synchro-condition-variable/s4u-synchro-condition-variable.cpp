/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mutex>           /* std::mutex and std::lock_guard */
#include <simgrid/s4u.hpp> /* All of S4U */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

std::string data;
bool done = false;

static void worker_fun(simgrid::s4u::ConditionVariablePtr cv, simgrid::s4u::MutexPtr mutex)
{
  std::unique_lock<simgrid::s4u::Mutex> lock(*mutex);

  XBT_INFO("Start processing data which is '%s'.", data.c_str());
  data += std::string(" after processing");

  // Send data back to main()
  XBT_INFO("Signal to master that the data processing is completed, and exit.");

  done = true;
  cv->notify_one();
}

static void master_fun()
{
  auto mutex  = simgrid::s4u::Mutex::create();
  auto cv     = simgrid::s4u::ConditionVariable::create();
  data        = std::string("Example data");
  auto worker = simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), worker_fun, cv, mutex);

  // wait for the worker
  cv->wait(std::unique_lock<simgrid::s4u::Mutex>(*mutex), []() { return done; });
  XBT_INFO("data is now '%s'.", data.c_str());

  worker->join();
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor::create("main", e.host_by_name("Tremblay"), master_fun);
  e.run();

  return 0;
}
