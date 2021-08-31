/* Copyright (c) 2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to serialize a set of communications going through a link
 *
 * As for the other asynchronous examples, the sender initiates all the messages it wants to send and
 * pack the resulting simgrid::s4u::CommPtr objects in a vector.
 * At the same time, the receiver starts receiving all messages asynchronously. Without serialization,
 * all messages would be received at the same timestamp in the receiver.
 *
 * However, as they will be serialized in a link of the platform, the messages arrive 2 by 2.
 *
 * The sender then blocks until all ongoing communication terminate, using simgrid::s4u::Comm::wait_all()
 */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_exec_failure, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void dispatcher(sg4::Host* host1, sg4::Host* host2)
{
  std::vector<sg4::ExecPtr> pending_execs;
  XBT_INFO("Initiating asynchronous exec on %s", host1->get_cname());
  auto exec1 = sg4::this_actor::exec_init(20)->set_host(host1);
  pending_execs.push_back(exec1);
  exec1->start();
  XBT_INFO("Initiating asynchronous exec on %s", host2->get_cname());
  auto exec2 = sg4::this_actor::exec_init(20)->set_host(host2);
  pending_execs.push_back(exec2);
  exec2->start();

  XBT_INFO("Calling wait_any..");
  try {
    long index = sg4::Exec::wait_any(pending_execs);
    XBT_INFO("Wait any returned index %ld (exec on %s)", index, pending_execs.at(index)->get_host()->get_cname());
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("Dispatcher has experienced a host failure exception, so it knows that something went wrong");
    XBT_INFO("Now it needs to figure out which of the two execs failed by looking at their state");
  }

  XBT_INFO("Exec on %s has state: %s", pending_execs[0]->get_host()->get_cname(), pending_execs[0]->get_state_str());
  XBT_INFO("Exec on %s has state: %s", pending_execs[1]->get_host()->get_cname(), pending_execs[1]->get_state_str());

  try {
    pending_execs[1]->wait();
  } catch (const simgrid::HostFailureException& e) {
    XBT_INFO("Waiting on a FAILED exec raises an exception: '%s'", e.what());
  }
  pending_execs.pop_back();
  XBT_INFO("Wait for remaining exec, just to be nice");
  simgrid::s4u::Exec::wait_any(pending_execs);
  XBT_INFO("Dispatcher ends");
}

static void host_killer(sg4::Host* to_kill)
{
  XBT_INFO("HostKiller  sleeping 10 seconds...");
  sg4::this_actor::sleep_for(10.0);
  XBT_INFO("HostKiller turning off host %s", to_kill->get_cname());
  to_kill->turn_off();
  XBT_INFO("HostKiller ends");
}

int main(int argc, char** argv)
{

  sg4::Engine engine(&argc, argv);

  auto* zone  = sg4::create_full_zone("AS0");
  auto* host1 = zone->create_host("Host1", "1f");
  auto* host2 = zone->create_host("Host2", "1f");
  zone->seal();

  sg4::Actor::create("Dispatcher", host1, dispatcher, host1, host2);
  sg4::Actor::create("HostKiller", host1, host_killer, host2)->daemonize();

  engine.run();

  return 0;
}
