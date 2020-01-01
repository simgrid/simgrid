/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <mutex> /* std::mutex and std::lock_guard */
#include "simgrid/s4u.hpp" /* All of S4U */

constexpr int NB_ACTOR = 6;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

/* This worker uses a classical mutex */
static void worker(simgrid::s4u::MutexPtr mutex, int& result)
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

static void workerLockGuard(simgrid::s4u::MutexPtr mutex, int& result)
{
  // Simply use the std::lock_guard like this
  // It's like a lock() that would do the unlock() automatically when getting out of scope
  std::lock_guard<simgrid::s4u::Mutex> lock(*mutex);

  // then you are in a safe zone
  XBT_INFO("Hello s4u, I'm ready to compute after a lock_guard");
  // update the results
  result += 1;
  XBT_INFO("I'm done, good bye");

  // Nothing specific here: the unlock will be automatic
}

static void master()
{
  int result = 0;
  simgrid::s4u::MutexPtr mutex = simgrid::s4u::Mutex::create();

  for (int i = 0; i < NB_ACTOR * 2 ; i++) {
    // To create a worker use the static method simgrid::s4u::Actor.
    if((i % 2) == 0 )
      simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), workerLockGuard, mutex,
                                  std::ref(result));
    else
      simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Tremblay"), worker, mutex, std::ref(result));
  }

  simgrid::s4u::this_actor::sleep_for(10);
  XBT_INFO("Results is -> %d", result);
}

int main(int argc, char **argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor::create("main", simgrid::s4u::Host::by_name("Tremblay"), master);
  e.run();

  return 0;
}
