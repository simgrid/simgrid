/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

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
#include <xbt/synchro.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

static int receiver(const char* box_name)
{
  int* payload;
  auto mb = simgrid::s4u::Mailbox::by_name(box_name);

  payload = static_cast<int*>(mb->get());
  MC_assert(*payload == 0);
  free(payload);

  payload = static_cast<int*>(mb->get());
  MC_assert(*payload == 1);
  free(payload);

  return 0;
}

static int sender(const char* box_name, simgrid::s4u::MutexPtr mutex, int value)
{
  int* payload = new int(value);
  auto mb      = simgrid::s4u::Mailbox::by_name(box_name);

#ifndef DISABLE_THE_MUTEX
  mutex->lock();
#endif

  mb->put(static_cast<void*>(payload), 8);

#ifndef DISABLE_THE_MUTEX
  mutex->unlock();
#endif
  return 0;
}

int main(int argc, char* argv[])
{
  XBT_ATTRIB_UNUSED simgrid::s4u::MutexPtr mutex;
#ifndef DISABLE_THE_MUTEX
  mutex = simgrid::s4u::Mutex::create();
#endif

  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("Tremblay"), sender, "box", mutex, 1);
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("Tremblay"), sender, "box", mutex, 2);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("Jupiter"), receiver, "box");

#ifndef DISABLE_THE_MUTEX
  mutex = simgrid::s4u::Mutex::create();
#endif

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
