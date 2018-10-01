/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

#include <memory>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

// This example implements a one-time use barrier with 1 semaphore and 1 mutex.

static void worker(simgrid::s4u::SemaphorePtr semaphore, simgrid::s4u::MutexPtr mutex, int process_count, std::shared_ptr<int> count)
{
  mutex->lock();
  XBT_INFO("Got mutex. Incrementing count.");
  XBT_INFO("Count is %d", *count);
  *count = (*count) + 1;
  XBT_INFO("Count is now %d. Process count is %d.", *count, process_count);

  if (*count == process_count) {
    XBT_INFO("Releasing the semaphore %d times.", process_count-1);
    for (int i = 0; i < process_count-1; i++) {
      semaphore->release();
    }

    XBT_INFO("Releasing mutex.");
    mutex->unlock();
  }
  else {
    XBT_INFO("Releasing mutex.");
    mutex->unlock();
    XBT_INFO("Acquiring semaphore.");
    semaphore->acquire();
  }

  XBT_INFO("Bye!");
}

static void master(unsigned int process_count)
{
  simgrid::s4u::SemaphorePtr semaphore = simgrid::s4u::Semaphore::create(0);
  simgrid::s4u::MutexPtr mutex = simgrid::s4u::Mutex::create();
  std::shared_ptr<int> count(new int);
  *count = 0;

  XBT_INFO("Spawning %d workers", process_count);
  for (unsigned int i = 0; i < process_count; i++) {
    simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), worker, semaphore, mutex, process_count, count);
  }
}

int main(int argc, char **argv)
{
  // Parameter: Number of processes in the barrier
  xbt_assert(argc >= 2, "Usage: %s <process-count>\n", argv[0]);
  unsigned int process_count = std::stoi(argv[1]);
  xbt_assert(process_count > 0, "<process-count> must be greater than 0");

  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), master, process_count);
  e.run();

  return 0;
}
