/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// This example implements a simple producer/consumer schema,
// passing a bunch of items from one to the other

#include "simgrid/s4u.hpp"

#include <memory>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(sem_test, "Simple test of the semaphore");

static void producer(std::string* buffer, sg4::SemaphorePtr sem_empty, sg4::SemaphorePtr sem_full,
                     const std::vector<std::string>& args)
{
  for (auto const& str : args) {
    sem_empty->acquire();
    XBT_INFO("Pushing '%s'", str.c_str());
    *buffer = str;
    sem_full->release();
  }

  XBT_INFO("Bye!");
}
static void consumer(const std::string* buffer, sg4::SemaphorePtr sem_empty, sg4::SemaphorePtr sem_full)
{
  std::string str;
  do {
    sem_full->acquire();
    str = *buffer;
    XBT_INFO("Receiving '%s'", str.c_str());
    sem_empty->release();
  } while (str != "");

  XBT_INFO("Bye!");
}

int main(int argc, char **argv)
{
  std::vector<std::string> args({"one", "two", "three", ""});
  sg4::Engine e(&argc, argv);
  e.load_platform(argc > 1 ? argv[1] : "../../platforms/two_hosts.xml");

  std::string buffer;                         /* Where the data is exchanged */
  auto sem_empty = sg4::Semaphore::create(1); /* indicates whether the buffer is empty */
  auto sem_full  = sg4::Semaphore::create(0); /* indicates whether the buffer is full */

  sg4::Actor::create("producer", e.host_by_name("Tremblay"), producer, &buffer, sem_empty, sem_full, std::cref(args));
  sg4::Actor::create("consumer", e.host_by_name("Jupiter"), consumer, &buffer, sem_empty, sem_full);
  e.run();

  return 0;
}
