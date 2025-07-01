/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** Switch the system thread hosting our maestro.
 *
 *  That's a very advanced example in which we move the maestro context to another system thread.
 *  Not many users need it (maybe only one, actually), but this example is also a regression test.
 *
 *  This example is in C++ because we use C++11 threads to ensure that the feature is working as
 *  expected. You can still use that feature from a C code.
 */

#include "simgrid/Exception.hpp"
#include "simgrid/actor.h"
#include "simgrid/s4u.hpp"

#include <string>
#include <thread>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

const std::thread::id root_id = std::this_thread::get_id();

static void ensure_root_tid()
{
  std::thread::id this_id = std::this_thread::get_id();
  xbt_assert(root_id == this_id, "I was supposed to be the main thread");
  XBT_INFO("I am the main thread, as expected");
}
static void ensure_other_tid()
{
  std::thread::id this_id = std::this_thread::get_id();
  xbt_assert(this_id != root_id, "I was NOT supposed to be the main thread");
  XBT_INFO("I am not the main thread, as expected");
}

static void sender()
{
  ensure_root_tid();
  auto* payload = new std::string("some message");
  sg4::Mailbox::by_name("some mailbox")->put(payload, 10e8);
}

static void receiver()
{
  ensure_other_tid();

  sg4::Mailbox::by_name("some mailbox")->get_unique<std::string>();
  XBT_INFO("Task received");
}

static void maestro(void* /* data */)
{
  sg4::Engine& e = *sg4::this_actor::get_engine();

  ensure_other_tid();
  e.host_by_name("Jupiter")->add_actor("receiver", receiver);
  e.run();
}

/** Main function */
int main(int argc, char* argv[])
{
  /* Specify which code should be executed by maestro on another thread, once this current thread is affected to an
   * actor by the subsequent sg_actor_attach(). This must be done before the creation of the engine. */
  simgrid_set_maestro(maestro, nullptr);

  sg4::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n"
                        "example: %s ../platforms/small_platform.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]);

  /* Become one of the simulated actors (must be done after the platform creation, or the host won't exist). */
  sg_actor_attach_pthread("sender", nullptr, e.host_by_name("Tremblay"));

  ensure_root_tid(); // Only useful in this test: we ensure that SimGrid is not broken and that this code is executed in
                     // the correct system thread

  // Execute the sender code. The root thread was actually turned into a regular actor
  sender();

  sg_actor_detach(); // The root thread becomes maestro again (as proved by the output)
  XBT_INFO("Detached");
  ensure_root_tid();

  return 0;
}
