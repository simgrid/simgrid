/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// This example implements a simple producer/consumer schema,
// passing a bunch of items from one to the other

#include "simgrid/s4u.hpp"

#include <memory>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

const char* buffer;                                                        /* Where the data is exchanged */
simgrid::s4u::SemaphorePtr sem_empty = simgrid::s4u::Semaphore::create(1); /* indicates whether the buffer is empty */
simgrid::s4u::SemaphorePtr sem_full  = simgrid::s4u::Semaphore::create(0); /* indicates whether the buffer is full */

static void producer(const std::vector<std::string>* args)
{
  for (auto str : *args) {
    sem_empty->acquire();
    XBT_INFO("Pushing '%s'", str.c_str());
    buffer = str.c_str();
    sem_full->release();
  }

  XBT_INFO("Bye!");
}
static void consumer()
{
  std::string str;
  do {
    sem_full->acquire();
    str = buffer;
    XBT_INFO("Receiving '%s'", str.c_str());
    sem_empty->release();
  } while (str != "");

  XBT_INFO("Bye!");
}

int main(int argc, char **argv)
{
  std::vector<std::string> args = std::vector<std::string>({"one", "two", "three", ""});
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform("../../platforms/two_hosts.xml");
  simgrid::s4u::Actor::create("producer", simgrid::s4u::Host::by_name("Tremblay"), producer, &args);
  simgrid::s4u::Actor::create("consumer", simgrid::s4u::Host::by_name("Jupiter"), consumer);
  e.run();

  return 0;
}
