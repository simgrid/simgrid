/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/sysdep.h>

#include "simgrid/s4u.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");

class Worker {
public:
  void operator()() {
    XBT_INFO("Hello s4u, I'm ready to serve");
    char *msg = static_cast<char*>(simgrid::s4u::this_actor::recv(
      *simgrid::s4u::Mailbox::byName("worker")));
    XBT_INFO("I received '%s'",msg);
    XBT_INFO("I'm done. See you.");
  }
};

class Master {
public:
  void operator()() {
    const char *msg = "GaBuZoMeu";
    XBT_INFO("Hello s4u, I have something to send");
    simgrid::s4u::this_actor::send(*simgrid::s4u::Mailbox::byName("worker"), xbt_strdup(msg), strlen(msg));
    XBT_INFO("I'm done. See you.");
  }
};


int main(int argc, char **argv) {
  simgrid::s4u::Engine *e = new simgrid::s4u::Engine(&argc,argv);
  e->loadPlatform("../../platforms/two_hosts.xml");
  new simgrid::s4u::Actor("worker", simgrid::s4u::Host::by_name("Tremblay"), Worker());
  new simgrid::s4u::Actor("master", simgrid::s4u::Host::by_name("Jupiter"), 0, Master());
  e->run();
  return 0;
}
