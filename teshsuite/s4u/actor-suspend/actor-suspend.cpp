/* Copyright (c) 2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// This is the MWE of https://framagit.org/simgrid/simgrid/-/issues/50
// The problem was occuring when suspending an actor that will be executed later in the same scheduling round

#include <iostream>
#include <simgrid/s4u.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(mwe, "Minimum Working Example");

simgrid::s4u::ActorPtr receiver;

class Receiver {

public:
  void operator()()
  {
    XBT_INFO("Starting.");
    simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("receiver");
    void* data                     = (void*)2;
    data                           = mailbox->get();
    xbt_die("get() has returned (even though it shouldn't!) with a %s message",
            (data == nullptr ? "null" : "non-null"));
  }
};

class Suspender {

public:
  void operator()()
  {

    // If we sleep a bit here, this MWE works because the suspender is not trying to suspend someone executed later in
    // the same scheduling round simgrid::s4u::this_actor::sleep_for(0.01);

    XBT_INFO("Suspend the receiver...");
    receiver->suspend();
    XBT_INFO("Resume the receiver...");
    receiver->resume();

    XBT_INFO("Sleeping 10 sec...");
    simgrid::s4u::this_actor::sleep_for(10);
    XBT_INFO("Done!");
  }
};

int main(int argc, char** argv)
{

  simgrid::s4u::Engine* engine = new simgrid::s4u::Engine(&argc, argv);

  engine->load_platform(argv[1]);
  simgrid::s4u::Host* host = simgrid::s4u::Host::by_name("Tremblay");

  auto suspender = simgrid::s4u::Actor::create("Suspender", host, Suspender());
  receiver       = simgrid::s4u::Actor::create("Receiver", host, Receiver());

  engine->run();

  return 0;
}