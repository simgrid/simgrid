/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(mwe, "Minimum Working Example");

simgrid::s4u::MutexPtr mutex;

class MutexHoldingActor {
public:
  void operator()()
  {

    simgrid::s4u::this_actor::sleep_for(5);
    XBT_INFO("Locking mutex");
    mutex->lock();
    XBT_INFO("Sleeping for ever");
    simgrid::s4u::this_actor::sleep_for(1000);
    mutex->unlock();
  }
};

class NonDaemonizedActor {
public:
  void operator()()
  {

    simgrid::s4u::this_actor::sleep_for(10);
    XBT_INFO("Terminating");
  }
};

int main(int argc, char** argv)
{
  auto engine = simgrid::s4u::Engine(&argc, argv);
  engine.load_platform(argv[1]);

  mutex = simgrid::s4u::Mutex::create();

  auto host = engine.get_all_hosts()[0];
  host->add_actor("MutexHoldingActor", MutexHoldingActor())->daemonize();
  host->add_actor("NonDaemonizedActor", NonDaemonizedActor());

  engine.run();
  XBT_INFO("Returned from engine->run()");

  return 0;
}
