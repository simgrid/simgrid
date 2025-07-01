/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// This example implements a simple producer/consumer schema, passing a bunch of items from one to the other,
// hopefully implemented in a way that resists resource failures.

#include <simgrid/s4u.hpp>
#include <xbt/config.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(sem_monkey, "Simple test of the semaphore");

static simgrid::config::Flag<int> cfg_item_count{"item-count", "Amount of items that must be exchanged to succeed", 2};
static simgrid::config::Flag<double> cfg_deadline{"deadline", "When to fail the simulation (infinite loop detection)",
                                                  120};

struct SharedBuffer {
  int value                   = 0;                         /* Where the data is exchanged */
  sg4::SemaphorePtr sem_empty = sg4::Semaphore::create(1); /* indicates whether the buffer is empty */
  sg4::SemaphorePtr sem_full  = sg4::Semaphore::create(0); /* indicates whether the buffer is full */
};

// A stack to keep track of semaphores.  When destroyed, semaphores remaining on stack are automatically released.
class SemStack {
  std::vector<sg4::Semaphore*> to_release;

public:
  void clear()
  {
    while (not to_release.empty()) {
      auto* sem = to_release.back();
      XBT_INFO("Go release a semaphore");
      sem->release();
      XBT_INFO("Released a semaphore on reboot. It's now %d", sem->get_capacity());
      to_release.pop_back();
    }
  }
  void push(const sg4::SemaphorePtr& sem) { to_release.push_back(sem.get()); }
  void pop() { to_release.pop_back(); }
};

static void producer(SharedBuffer& buf)
{
  static int todo    = cfg_item_count; // remaining amount of items to exchange
  static SemStack to_release;
  bool rebooting = sg4::Actor::self()->get_restart_count() > 0;

  XBT_INFO("Producer %s", rebooting ? "rebooting" : "booting");
  if (not rebooting) // Starting for the first time
    sg4::this_actor::on_exit(
        [](bool forcefully) { XBT_INFO("Producer dying %s.", forcefully ? "forcefully" : "peacefully"); });
  to_release.clear();

  while (todo > 0) {
    sg4::this_actor::sleep_for(1); // Give a chance to the monkey to kill this actor at this point

    while (buf.sem_empty->acquire_timeout(10)) {
      XBT_INFO("Timeouted");
      xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
                 "Failed to exchange all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);
    }
    to_release.push(buf.sem_empty);
    XBT_INFO("sem_empty acquired");

    sg4::this_actor::sleep_for(1); // Give a chance to the monkey to kill this actor at this point

    XBT_INFO("Pushing item %d", todo - 1);
    buf.value = todo - 1;
    buf.sem_full->release();
    to_release.pop();
    XBT_INFO("sem_empty removed from to_release");
    todo--;
  }
}

static void consumer(const SharedBuffer& buf)
{
  static SemStack to_release;
  bool rebooting = sg4::Actor::self()->get_restart_count() > 0;

  XBT_INFO("Consumer %s", rebooting ? "rebooting" : "booting");
  if (not rebooting) // Starting for the first time
    sg4::this_actor::on_exit(
        [](bool forcefully) { XBT_INFO("Consumer dying %s.", forcefully ? "forcefully" : "peacefully"); });
  to_release.clear();

  int item;
  do {
    sg4::this_actor::sleep_for(0.75); // Give a chance to the monkey to kill this actor at this point

    while (buf.sem_full->acquire_timeout(10)) {
      XBT_INFO("Timeouted");
      xbt_assert(sg4::Engine::get_clock() < cfg_deadline,
                 "Failed to exchange all tasks in less than %d seconds. Is this an infinite loop?", (int)cfg_deadline);
    }
    to_release.push(buf.sem_full);

    sg4::this_actor::sleep_for(0.75); // Give a chance to the monkey to kill this actor at this point

    item = buf.value;
    XBT_INFO("Receiving item %d", item);
    buf.sem_empty->release();
    to_release.pop();
  } while (item != 0);

  XBT_INFO("Bye!");
}

int main(int argc, char** argv)
{
  sg4::Engine e(&argc, argv);

  auto* rootzone = e.get_netzone_root();
  auto* paul     = rootzone->add_host("Paul", 1e9);
  auto* carol    = rootzone->add_host("Carol", 1e9);
  auto* link     = rootzone->add_link("link", "1MBps")->set_latency("24us")->seal();
  rootzone->add_route(paul, carol, {link});

  SharedBuffer buffer;
  paul->add_actor("producer", producer, std::ref(buffer))->set_auto_restart();
  carol->add_actor("consumer", consumer, std::cref(buffer))->set_auto_restart();
  e.run();

  return 0;
}
