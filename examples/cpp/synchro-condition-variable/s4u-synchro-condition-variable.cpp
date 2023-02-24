/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mutex>           /* std::mutex and std::scoped_lock */
#include <simgrid/s4u.hpp> /* All of S4U */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");
namespace sg4 = simgrid::s4u;

static void worker_fun(sg4::ConditionVariablePtr cv, sg4::MutexPtr mutex, std::string& data, bool& done)
{
  const std::scoped_lock lock(*mutex);

  XBT_INFO("Start processing data which is '%s'.", data.c_str());
  data += " after processing";

  // Send data back to main()
  XBT_INFO("Signal to master that the data processing is completed, and exit.");

  done = true;
  cv->notify_one();
}

static void master_fun()
{
  auto mutex  = sg4::Mutex::create();
  auto cv     = sg4::ConditionVariable::create();
  std::string data = "Example data";
  bool done        = false;

  auto worker = sg4::Actor::create("worker", sg4::Host::by_name("Jupiter"), worker_fun, cv, mutex, std::ref(data),
                                   std::ref(done));

  // wait for the worker
  cv->wait(std::unique_lock(*mutex), [&done]() { return done; });
  XBT_INFO("data is now '%s'.", data.c_str());

  worker->join();
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);
  e.load_platform("../../platforms/two_hosts.xml");
  sg4::Actor::create("main", e.host_by_name("Tremblay"), master_fun);
  e.run();

  return 0;
}
