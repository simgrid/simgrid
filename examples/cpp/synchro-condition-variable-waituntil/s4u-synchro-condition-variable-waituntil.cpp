/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mutex>           /* std::mutex and std::lock_guard */
#include <simgrid/s4u.hpp> /* All of S4U */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

simgrid::s4u::MutexPtr mtx = nullptr;
simgrid::s4u::ConditionVariablePtr cv = nullptr;
bool ready = false;

static void competitor(int id)
{
  XBT_INFO("Entering the race...");
  std::unique_lock<simgrid::s4u::Mutex> lck(*mtx);
  while (not ready) {
    auto now = simgrid::s4u::Engine::get_clock();
    if (cv->wait_until(lck, now + (id+1)*0.25) == std::cv_status::timeout) {
      XBT_INFO("Out of wait_until (timeout)");
    }
    else {
      XBT_INFO("Out of wait_until (YAY!)");
    }
  }
  XBT_INFO("Running!");
}

static void go()
{
  XBT_INFO("Are you ready? ...");
  simgrid::s4u::this_actor::sleep_for(3);
  std::unique_lock<simgrid::s4u::Mutex> lck(*mtx);
  XBT_INFO("Go go go!");
  ready = true;
  cv->notify_all();
}

static void main_actor()
{
  mtx = simgrid::s4u::Mutex::create();
  cv = simgrid::s4u::ConditionVariable::create();

  auto host = simgrid::s4u::this_actor::get_host();
  for (int i = 0; i < 10; ++i)
    simgrid::s4u::Actor::create("competitor", host, competitor, i);
  simgrid::s4u::Actor::create("go", host, go);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform("../../platforms/small_platform.xml");

  simgrid::s4u::Actor::create("main", e.host_by_name("Tremblay"), main_actor);

  e.run();
  return 0;
}
