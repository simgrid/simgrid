/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup S4U_examples
 *
 *  - <b>maestro-set/maestro-set.cpp: Switch the system thread hosting our maestro</b>.
 *    That's a very advanced example in which we move the maestro thread to another process.
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

std::thread::id root_id;

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
  std::string* payload = new std::string("some message");
  simgrid::s4u::Mailbox::by_name("some mailbox")->put((void*)payload, 10e8);
}

static void receiver()
{
  ensure_other_tid();

  const std::string* payload = static_cast<std::string*>(simgrid::s4u::Mailbox::by_name("some mailbox")->get());
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
  root_id = std::this_thread::get_id();

  SIMIX_set_maestro(maestro, NULL);
  simgrid::s4u::Engine e(&argc, argv);

  if (argc != 2) {
    XBT_CRITICAL("Usage: %s platform_file\n", argv[0]);
    xbt_die("example: %s ../platforms/small_platform.xml\n", argv[0]);
  }

  e.load_platform(argv[1]);

  /* Become one of the simulated process.
   *
   * This must be done after the creation of the platform because we are depending attaching to a host.*/
  sg_actor_attach("sender", nullptr, simgrid::s4u::Host::by_name("Tremblay"), nullptr);
  ensure_root_tid();

  // Execute the sender code:
  sender();

  sg_actor_detach(); // Become root thread again
  XBT_INFO("Detached");
  ensure_root_tid();

  return 0;
}
