/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <functional>
#include <mutex>

#include <xbt/sysdep.h>

#include "simgrid/s4u.h"

#define NB_ACTOR 2

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

static void worker(simgrid::s4u::Mutex mutex, int& result)
{
  // Do the calculation
  simgrid::s4u::this_actor::execute(1000);

  // lock the mutex before enter in the critical section
  std::lock_guard<simgrid::s4u::Mutex> lock(mutex);
  XBT_INFO("Hello s4u, I'm ready to compute");

  // And finaly add it to the results
  result += 1;
  XBT_INFO("I'm done, good bye");
}

static void workerLockGuard(simgrid::s4u::Mutex mutex, int& result)
{
  simgrid::s4u::this_actor::execute(1000);

  // Simply use the std::lock_guard like this
  std::lock_guard<simgrid::s4u::Mutex> lock(mutex);

  // then you are in a safe zone
  XBT_INFO("Hello s4u, I'm ready to compute");
  // update the results
  result += 1;
  XBT_INFO("I'm done, good bye");
}

static void master()
{
  int result = 0;
  simgrid::s4u::Mutex mutex;

  for (int i = 0; i < NB_ACTOR * 2 ; i++) {
    // To create a worker use the static method simgrid::s4u::Actor.
    if((i % 2) == 0 )
      simgrid::s4u::Actor("worker", simgrid::s4u::Host::by_name("Jupiter"),  workerLockGuard, mutex, std::ref(result));
    else
      simgrid::s4u::Actor("worker", simgrid::s4u::Host::by_name("Tremblay"), worker,          mutex, std::ref(result));
  }

  simgrid::s4u::this_actor::sleep(10);
  XBT_INFO("Results is -> %d", result);
}

int main(int argc, char **argv)
{
  simgrid::s4u::Engine *e = new simgrid::s4u::Engine(&argc,argv);
  e->loadPlatform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor("main", simgrid::s4u::Host::by_name("Tremblay"), master);
  e->run();
  return 0;
}
