/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// This example implements a simple producer/consumer schema, passing a bunch of items from one to the other,
// hopefully implemented in a way that resists resource failures.

#include <simgrid/s4u.hpp>
#include <xbt/config.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(sem_monkey, "Simple test of the semaphore");

int buffer;                                              /* Where the data is exchanged */
sg4::SemaphorePtr sem_empty = sg4::Semaphore::create(1); /* indicates whether the buffer is empty */
sg4::SemaphorePtr sem_full  = sg4::Semaphore::create(0); /* indicates whether the buffer is full */

static simgrid::config::Flag<int> cfg_item_count{"item-count", "Amount of items that must be exchanged to succeed", 2};
static simgrid::config::Flag<double> cfg_deadline{"deadline", "When to fail the simulation (infinite loop detection)",
                                                  120};

int todo; // remaining amount of items to exchange

// A stack to keep track of semaphores.  When destroyed, semaphores remaining on stack are automatically released.
class SemStack {
  std::vector<sg4::Semaphore*> to_release;

public:
  void push(sg4::SemaphorePtr& sem) { to_release.push_back(sem.get()); }
  void pop() { to_release.pop_back(); }
  ~SemStack()
  {
    for (auto* sem : to_release) {
      sem->release();
      XBT_INFO("Released a semaphore on exit. It's now %d", sem->get_capacity());
    }
  }
};

static void producer()
{
  static bool inited = false;
  SemStack to_release;
  XBT_INFO("Producer %s", inited ? "rebooting" : "booting");

  if (not inited) {
    sg4::this_actor::on_exit(
        [](bool forcefully) { XBT_INFO("Producer dying %s.", forcefully ? "forcefully" : "peacefully"); });
    inited = true;
  }

  while (todo > 0) {
    xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
               "Failed to exchange all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);

    sg4::this_actor::sleep_for(1); // Give a chance to the monkey to kill this actor at this point

    while (sem_empty->acquire_timeout(10))
      XBT_INFO("Timeouted");
    to_release.push(sem_empty);
    XBT_INFO("sem_empty acquired");

    sg4::this_actor::sleep_for(1); // Give a chance to the monkey to kill this actor at this point

    XBT_INFO("Pushing item %d", todo - 1);
    buffer = todo - 1;
    sem_full->release();
    to_release.pop();
    XBT_INFO("sem_empty removed from to_release");
    todo--;
  }
}
static void consumer()
{
  SemStack to_release;

  static bool inited = false;
  XBT_INFO("Consumer %s", inited ? "rebooting" : "booting");
  if (not inited) {
    sg4::this_actor::on_exit(
        [](bool forcefully) { XBT_INFO("Consumer dying %s.", forcefully ? "forcefully" : "peacefully"); });
    inited = true;
  }

  int item;
  do {
    xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
               "Failed to exchange all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);

    sg4::this_actor::sleep_for(0.75); // Give a chance to the monkey to kill this actor at this point

    while (sem_full->acquire_timeout(10))
      XBT_INFO("Timeouted");
    to_release.push(sem_full);

    sg4::this_actor::sleep_for(0.75); // Give a chance to the monkey to kill this actor at this point

    item = buffer;
    XBT_INFO("Receiving item %d", item);
    sem_empty->release();
    to_release.pop();
  } while (item != 0);

  XBT_INFO("Bye!");
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);

  todo           = cfg_item_count;
  auto* rootzone = sg4::create_full_zone("root");
  auto* paul     = rootzone->create_host("Paul", 1e9);
  auto* carol    = rootzone->create_host("Carol", 1e9);
  sg4::LinkInRoute link(rootzone->create_link("link", "1MBps")->set_latency("24us")->seal());
  rootzone->add_route(paul->get_netpoint(), carol->get_netpoint(), nullptr, nullptr, {link}, true);

  sg4::Actor::create("producer", paul, producer)->set_auto_restart();
  sg4::Actor::create("consumer", carol, consumer)->set_auto_restart();
  e.run();

  return 0;
}
