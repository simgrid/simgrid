/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

int tasks_done = 0;

XBT_ATTRIB_NORETURN static void actor_daemon()
{
  const simgrid::s4u::Host* host = simgrid::s4u::Host::current();
  XBT_INFO("  Start daemon on %s (%f)", host->get_cname(), host->get_speed());
  for (;;) {
    XBT_INFO("  Execute daemon");
    simgrid::s4u::this_actor::execute(host->get_speed());
    tasks_done++;
  }
  XBT_INFO("  daemon done. See you!");
}

static void commTX()
{
  XBT_INFO("  Start TX");
  std::string* payload = new std::string("COMM");
  simgrid::s4u::Mailbox::by_name("comm")->put_init(payload, 100000000)->detach();
  // We should wait a bit (if not the process will end before the communication, hence an exception on the other side).
  try {
    simgrid::s4u::this_actor::sleep_for(30);
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("The host has died ... as expected.");
  }
  delete payload;

  XBT_INFO("  TX done");
}

static void commRX()
{
  const std::string* payload = nullptr;
  XBT_INFO("  Start RX");
  try {
    payload = static_cast<std::string*>(simgrid::s4u::Mailbox::by_name("comm")->get());
    XBT_INFO("  Receive message: %s", payload->c_str());
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("  Receive message: HOST_FAILURE");
  } catch (const simgrid::NetworkFailureException&) {
    XBT_INFO("  Receive message: TRANSFER_FAILURE");
  }

  delete payload;
  XBT_INFO("  RX Done");
}

static void test_launcher(int test_number)
{
  simgrid::s4u::Host* jupiter = simgrid::s4u::Host::by_name("Jupiter");
  simgrid::s4u::ActorPtr daemon;
  simgrid::s4u::VirtualMachine* vm0 = nullptr;

  switch (test_number) {
    case 1:
      // Create a process running a simple task on a host and turn the host off during the execution of the actor.
      XBT_INFO("Test 1:");
      XBT_INFO("  Create an actor on Jupiter");
      simgrid::s4u::Actor::create("actor_daemon", jupiter, actor_daemon);
      simgrid::s4u::this_actor::sleep_for(3);
      XBT_INFO("  Turn off Jupiter");
      jupiter->turn_off();
      simgrid::s4u::this_actor::sleep_for(10);
      XBT_INFO("Test 1 seems ok, cool !(#Actors: %zu, it should be 1; #tasks: %d)",
               simgrid::s4u::Engine::get_instance()->get_actor_count(), tasks_done);
      break;
    case 2:
      // Create an actorthat on a host that is turned off (this is not allowed)
      XBT_INFO("Test 2:");
      XBT_INFO("  Turn off Jupiter");
      // adsein: Jupiter is already off, hence nothing should happen
      // adsein: This can be one additional test, to check that you cannot shutdown twice a host
      jupiter->turn_off();
      try {
        simgrid::s4u::Actor::create("actor_daemon", jupiter, actor_daemon);
        simgrid::s4u::this_actor::sleep_for(10);
        XBT_INFO("  Test 2 does crash as it should. This message will not be displayed.");
      } catch (const simgrid::HostFailureException&) {
        xbt_die("Could not launch a new actor on failed host %s.", jupiter->get_cname());
      }
      break;
    case 3:
      // Create an actor running successive sleeps on a host and turn the host off during the execution of the actor.
      xbt_die("Test 3 is superseded by activity-lifecycle");
      break;
    case 4:
      XBT_INFO("Test 4 (turn off src during a communication) : Create an actor/task to make a communication between "
               "Jupiter and Tremblay and turn off Jupiter during the communication");
      jupiter->turn_on();
      simgrid::s4u::this_actor::sleep_for(10);
      simgrid::s4u::Actor::create("commRX", simgrid::s4u::Host::by_name("Tremblay"), commRX);
      simgrid::s4u::Actor::create("commTX", jupiter, commTX);
      XBT_INFO("  number of actors: %zu", simgrid::s4u::Engine::get_instance()->get_actor_count());
      simgrid::s4u::this_actor::sleep_for(10);
      XBT_INFO("  Turn Jupiter off");
      jupiter->turn_off();
      XBT_INFO("Test 4 is ok.  (number of actors : %zu, it should be 1 or 2 if RX has not been satisfied)."
               " An exception is raised when we turn off a node that has an actor sleeping",
               simgrid::s4u::Engine::get_instance()->get_actor_count());
      break;
    case 5:
      XBT_INFO("Test 5 (turn off dest during a communication : Create an actor/task to make a communication between "
               "Tremblay and Jupiter and turn off Jupiter during the communication");
      jupiter->turn_on();
      simgrid::s4u::this_actor::sleep_for(10);
      simgrid::s4u::Actor::create("commRX", jupiter, commRX);
      simgrid::s4u::Actor::create("commTX", simgrid::s4u::Host::by_name("Tremblay"), commTX);
      XBT_INFO("  number of actors: %zu", simgrid::s4u::Engine::get_instance()->get_actor_count());
      simgrid::s4u::this_actor::sleep_for(10);
      XBT_INFO("  Turn Jupiter off");
      jupiter->turn_off();
      XBT_INFO("Test 5 seems ok (number of actors: %zu, it should be 2)",
               simgrid::s4u::Engine::get_instance()->get_actor_count());
      break;
    case 6:
      XBT_INFO("Test 6: Turn on Jupiter, assign a VM on Jupiter, launch an actor inside the VM, and turn off the node");

      // Create VM0
      vm0 = new simgrid::s4u::VirtualMachine("vm0", jupiter, 1);
      vm0->start();

      daemon = simgrid::s4u::Actor::create("actor_daemon", vm0, actor_daemon);
      simgrid::s4u::Actor::create("actor_daemonJUPI", jupiter, actor_daemon);

      daemon->suspend();
      vm0->set_bound(90);
      daemon->resume();

      simgrid::s4u::this_actor::sleep_for(10);

      XBT_INFO("  Turn Jupiter off");
      jupiter->turn_off();
      XBT_INFO("  Shutdown vm0");
      vm0->shutdown();
      XBT_INFO("  Destroy vm0");
      vm0->destroy();
      XBT_INFO("Test 6 is also weird: when the node Jupiter is turned off once again, the VM and its daemon are not "
               "killed. However, the issue regarding the shutdown of hosted VMs can be seen a feature not a bug ;)");
      break;
    default:
      xbt_die("Unknown test case.");
  }

  XBT_INFO("  Test done. See you!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 3, "Usage: %s platform_file test_number\n\tExample: %s msg_platform.xml 1\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("test_launcher", simgrid::s4u::Host::by_name("Tremblay"), test_launcher,
                              std::stoi(argv[2]));

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
