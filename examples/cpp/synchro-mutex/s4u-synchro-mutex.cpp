/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp" /* All of S4U */
#include "xbt/config.hpp"
#include <mutex> /* std::mutex and std::lock_guard */

namespace sg4 = simgrid::s4u;

static simgrid::config::Flag<int> cfg_actor_count("actors", "How many pairs of actors should be started?", 6);

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

/* This worker uses a classical mutex */
static void worker(sg4::MutexPtr mutex, int& result)
{
  // lock the mutex before enter in the critical section
  mutex->lock();

  XBT_INFO("Hello s4u, I'm ready to compute after a regular lock");
  // And finally add it to the results
  result += 1;
  XBT_INFO("I'm done, good bye");

  // You have to unlock the mutex if you locked it manually.
  // Beware of exceptions preventing your unlock() from being executed!
  mutex->unlock();
}

static void workerLockGuard(sg4::MutexPtr mutex, int& result)
{
  // Simply use the std::lock_guard like this
  // It's like a lock() that would do the unlock() automatically when getting out of scope
  std::lock_guard<sg4::Mutex> lock(*mutex);

  // then you are in a safe zone
  XBT_INFO("Hello s4u, I'm ready to compute after a lock_guard");
  // update the results
  result += 1;
  XBT_INFO("I'm done, good bye");

  // Nothing specific here: the unlock will be automatic
}

static void master()
{
  /* Create the requested amount of actors pairs. Each pair has a specific mutex and cell in `result`. */
  int result[cfg_actor_count.get()];

  for (int i = 0; i < cfg_actor_count; i++) {
    result[i]           = 0;
    sg4::MutexPtr mutex = sg4::Mutex::create();
    sg4::Actor::create("worker", sg4::Host::by_name("Jupiter"), workerLockGuard, mutex, std::ref(result[i]));
    sg4::Actor::create("worker", sg4::Host::by_name("Tremblay"), worker, mutex, std::ref(result[i]));
  }

  sg4::this_actor::sleep_for(10);
  for (int i = 0; i < cfg_actor_count; i++)
    XBT_INFO("Result[%d] -> %d", i, result[i]);
}

int main(int argc, char **argv)
{
  sg4::Engine e(&argc, argv);
  e.load_platform("../../platforms/two_hosts.xml");
  sg4::Actor::create("main", e.host_by_name("Tremblay"), master);
  e.run();

  return 0;
}
