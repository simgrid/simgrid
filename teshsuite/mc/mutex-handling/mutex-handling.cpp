/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* In this test, we have two senders sending one message to a common receiver.
 * The receiver should be able to see any ordering between the two messages.
 * If we model-check the application with assertions on a specific order of
 * the messages (see the assertions in the receiver code), it should fail
 * because both ordering are possible.
 *
 * If the senders sends the message directly, the current version of the MC
 * finds that the ordering may differ and the MC find a counter-example.
 *
 * However, if the senders send the message in a mutex, the MC always let
 * the first process take the mutex because it thinks that the effect of
 * a mutex is purely local: the ordering of the messages is always the same
 * and the MC does not find the counter-example.
 */

#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "simgrid/s4u/Mutex.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int receiver(const char* box_name)
{
  auto mb = simgrid::s4u::Mailbox::by_name(box_name);
  std::unique_ptr<int> payload;

  payload = mb->get_unique<int>();
  MC_assert(*payload == 1);

  payload = mb->get_unique<int>();
  MC_assert(*payload == 2);

  return 0;
}

static int sender(const char* box_name, simgrid::s4u::MutexPtr mutex, int value)
{
  auto* payload = new int(value);
  auto mb      = simgrid::s4u::Mailbox::by_name(box_name);

  if (mutex)
    mutex->lock();

  mb->put(payload, 8);

  if (mutex)
    mutex->unlock();

  return 0;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n"
                       "\tExample: %s msg_platform.xml\n",
             argv[0], argv[0]);

  simgrid::s4u::MutexPtr mutex;
#ifndef DISABLE_THE_MUTEX
  mutex = simgrid::s4u::Mutex::create();
#endif

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("receiver", e.host_by_name("Jupiter"), receiver, "box");
  simgrid::s4u::Actor::create("sender", e.host_by_name("Boivin"), sender, "box", mutex, 1);
  simgrid::s4u::Actor::create("sender", e.host_by_name("Fafard"), sender, "box", mutex, 2);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
