/* Copyright (c) 2021-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This examples shows how to survive to host failure exceptions that occur when an host is turned off.
 *
 * The actors do not get notified when the host on which they run is turned off: they are just terminated
 * in this case, and their ``on_exit()`` callback gets executed.
 *
 * For remote executions on failing hosts however, any blocking operation such as ``exec`` or ``wait`` will
 * raise an exception that you can catch and react to, as illustrated in this example.
 */

#include <simgrid/s4u.hpp>
#include "simgrid/kernel/ProfileBuilder.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_exec_failure, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void dispatcher(std::vector<sg4::Host*> const& hosts)
{
  std::vector<sg4::ExecPtr> pending_execs;
  for (auto* host: hosts) {
    XBT_INFO("Initiating asynchronous exec on %s", host->get_cname());
    // Computing 20 flops on an host which speed is 1f takes 20 seconds (when it does not fail)
    auto exec = sg4::this_actor::exec_init(20)->set_host(host);
    pending_execs.push_back(exec);
    exec->start();
  }

  XBT_INFO("---------------------------------");
  XBT_INFO("Wait on the first exec, which host is turned off at t=10 by the another actor.");
  try {
    pending_execs[0]->wait();
    xbt_assert("This wait was not supposed to succeed.");
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("Dispatcher has experienced a host failure exception, so it knows that something went wrong.");
  }

  XBT_INFO("State of each exec:");
  for (auto const& exec : pending_execs) 
    XBT_INFO("  Exec on %s has state: %s", exec->get_host()->get_cname(), exec->get_state_str());

  XBT_INFO("---------------------------------");
  XBT_INFO("Wait on the second exec, which host is turned off at t=12 by the state profile.");
  try {
    pending_execs[1]->wait();
    xbt_assert("This wait was not supposed to succeed.");
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("Dispatcher has experienced a host failure exception, so it knows that something went wrong.");
  }
  XBT_INFO("State of each exec:");
  for (auto const& exec : pending_execs) 
    XBT_INFO("  Exec on %s has state: %s", exec->get_host()->get_cname(), exec->get_state_str());

  XBT_INFO("---------------------------------");
  XBT_INFO("Wait on the third exec, which should succeed.");
  try {
    pending_execs[2]->wait();
    XBT_INFO("No exception occured.");
  } catch (const simgrid::HostFailureException&) {
    xbt_assert("This wait was not supposed to fail.");
  }
  XBT_INFO("State of each exec:");
  for (auto const& exec : pending_execs) 
    XBT_INFO("  Exec on %s has state: %s", exec->get_host()->get_cname(), exec->get_state_str());
}

static void host_killer(sg4::Host* to_kill)
{
  sg4::this_actor::sleep_for(10.0);
  XBT_INFO("HostKiller turns off the host '%s'.", to_kill->get_cname());
  to_kill->turn_off();
}

int main(int argc, char** argv)
{
  sg4::Engine engine(&argc, argv);

  auto* zone  = sg4::create_full_zone("world");
  std::vector<sg4::Host*> hosts;
  for (const auto* name : {"Host1", "Host2", "Host3"}) {
    auto* host = zone->create_host(name, "1f");
    hosts.push_back(host);
  }
  /* Attaching a state profile (ie a list of events changing the on/off state of the resource) to host3.
   * The syntax of the profile (second parameter) is a list of: "date state\n"
   *   The R"(   )" thing is the C++ way of writing multiline strings, including literals \n.
   *     You'd have the same behavior by using "12 0\n20 1\n" instead.
   *   So here, the link is turned off at t=12 and back on at t=20.
   * The last parameter is the period of that profile, meaning that it loops after 30 seconds.
   */
  hosts[1]->set_state_profile(simgrid::kernel::profile::ProfileBuilder::from_string("profile name", R"(
12 0
20 1
)",                                                                               30));

  zone->seal();

  sg4::Actor::create("Dispatcher", hosts[2], dispatcher, hosts);
  sg4::Actor::create("HostKiller", hosts[2], host_killer, hosts[0]);

  engine.run();

  return 0;
}
