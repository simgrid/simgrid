/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup S4U_examples
 *
 *  - <b>maestro-set/maestro-set.cpp: Switch the system thread hosting our maestro</b>.
 *    That's a very advanced example in which we move the maestro context to another system thread.
 *    Not many users need it (maybe only one, actually), but this example is also a regression test.
 *
 *    This example is in C++ because we use C++11 threads to ensure that the feature is working as
 *    expected. You can still use that feature from a C code.
 */

#include "simgrid/Exception.hpp"
#include "simgrid/actor.h"
#include "simgrid/s4u.hpp"

#include <string>
#include <thread>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

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
  simgrid::s4u::Mailbox::by_name("some mailbox")->put((void*)payload, 10e8);
}

static void receiver()
{
  ensure_other_tid();

  const auto* payload = static_cast<std::string*>(simgrid::s4u::Mailbox::by_name("some mailbox")->get());
  XBT_INFO("Task received");
  delete payload;
}

static void maestro(void* /* data */)
{
  ensure_other_tid();
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("Jupiter"), receiver);
  simgrid::s4u::Engine::get_instance()->run();
}

/** Main function */
int main(int argc, char* argv[])
{
  /* Specify which code should be executed by maestro on another thread, once this current thread is affected to an
   * actor by the subsequent sg_actor_attach(). This must be done before the creation of the engine. */
  SIMIX_set_maestro(maestro, nullptr);

  simgrid::s4u::Engine e(&argc, argv);

  if (argc != 2) {
    XBT_CRITICAL("Usage: %s platform_file\n", argv[0]);
    xbt_die("example: %s ../platforms/small_platform.xml\n", argv[0]);
  }

  e.load_platform(argv[1]);

  /* Become one of the simulated process (must be done after the platform creation, or the host won't exist). */
  sg_actor_attach("sender", nullptr, simgrid::s4u::Host::by_name("Tremblay"), nullptr);

  ensure_root_tid(); // Only useful in this test: we ensure that simgrid is not broken and that this code is executed in
                     // the correct system thread

  // Execute the sender code. The root thread was actually turned into a regular actor
  sender();

  sg_actor_detach(); // The root thread becomes maestro again (as proved by the output)
  XBT_INFO("Detached");
  ensure_root_tid();

  return 0;
}
