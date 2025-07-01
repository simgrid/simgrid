/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

XBT_ATTRIB_NORETURN static void actor_daemon(int& tasks_done)
{
  const simgrid::s4u::Host* host = simgrid::s4u::Host::current();
  XBT_INFO("  Start daemon on %s (%f)", host->get_cname(), host->get_speed());
  for (;;) {
    XBT_INFO("  Execute daemon");
    simgrid::s4u::this_actor::execute(host->get_speed());
    tasks_done++;
  }
  xbt_die("  daemon done. See you!");
}

static void commTX()
{
  XBT_INFO("  Start TX");
  static std::string payload = "COMM";
  simgrid::s4u::Mailbox::by_name("comm")->put_init(&payload, 100000000)->detach();
  // We should wait a bit (if not the process will end before the communication, hence an exception on the other side).
  try {
    simgrid::s4u::this_actor::sleep_for(30);
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("The host has died ... as expected.");
  }

  XBT_INFO("  TX done");
}

static void commRX()
{
  XBT_INFO("  Start RX");
  try {
    const auto* payload = simgrid::s4u::Mailbox::by_name("comm")->get<std::string>();
    XBT_INFO("  Receive message: %s", payload->c_str());
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("  Receive message: HOST_FAILURE");
  } catch (const simgrid::NetworkFailureException&) {
    XBT_INFO("  Receive message: TRANSFER_FAILURE");
  }

  XBT_INFO("  RX Done");
}

static void test_launcher(int test_number)
{
  simgrid::s4u::Engine& e     = *simgrid::s4u::this_actor::get_engine();
  simgrid::s4u::Host* jupiter = simgrid::s4u::Host::by_name("Jupiter");
  simgrid::s4u::ActorPtr daemon;
  simgrid::s4u::VirtualMachine* vm0 = nullptr;
  int tasks_done                    = 0;

  switch (test_number) {
    case 1:
      // Create a process running a simple task on a host and turn the host off during the execution of the actor.
      XBT_INFO("Test 1:");
      XBT_INFO("  Create an actor on Jupiter");
      jupiter->add_actor("actor_daemon", actor_daemon, std::ref(tasks_done));
      simgrid::s4u::this_actor::sleep_for(3);
      XBT_INFO("  Turn off Jupiter");
      jupiter->turn_off();
      simgrid::s4u::this_actor::sleep_for(10);
      XBT_INFO("Test 1 seems ok, cool !(#Actors: %zu, it should be 1; #tasks: %d)",
               e.get_actor_count(), tasks_done);
      break;
    case 2:
      // Create an actor on an host that is turned off (this is not allowed)
      XBT_INFO("Test 2:");
      XBT_INFO("  Turn off Jupiter");
      // adsein: Jupiter is already off, hence nothing should happen
      // adsein: This can be one additional test, to check that you cannot shutdown twice a host
      jupiter->turn_off();
      try {
        jupiter->add_actor("actor_daemon",actor_daemon, std::ref(tasks_done));
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
      simgrid::s4u::Host::by_name("Tremblay")->add_actor("commRX", commRX);
      jupiter->add_actor("commTX", commTX);
      XBT_INFO("  number of actors: %zu", e.get_actor_count());
      simgrid::s4u::this_actor::sleep_for(10);
      XBT_INFO("  Turn Jupiter off");
      jupiter->turn_off();
      simgrid::s4u::this_actor::sleep_for(1); // Allow some time to the other actors to die
      XBT_INFO("Test 4 is ok.  (number of actors : %zu, it should be 1 or 2 if RX has not been satisfied)."
               " An exception is raised when we turn off a node that has an actor sleeping",
               e.get_actor_count());
      break;
    case 5:
      XBT_INFO("Test 5 (turn off dest during a communication : Create an actor/task to make a communication between "
               "Tremblay and Jupiter and turn off Jupiter during the communication");
      jupiter->turn_on();
      simgrid::s4u::this_actor::sleep_for(10);
      jupiter->add_actor("commRX", commRX);
      simgrid::s4u::Host::by_name("Tremblay")->add_actor("commTX", commTX);
      XBT_INFO("  number of actors: %zu", e.get_actor_count());
      simgrid::s4u::this_actor::sleep_for(10);
      XBT_INFO("  Turn Jupiter off");
      jupiter->turn_off();
      simgrid::s4u::this_actor::sleep_for(1); // Allow some time to the other actors to die
      XBT_INFO("Test 5 seems ok (number of actors: %zu, it should be 2)",
               e.get_actor_count());
      break;
    case 6:
      XBT_INFO("Test 6: Turn on Jupiter, assign a VM on Jupiter, launch an actor inside the VM, and turn off the node");

      // Create VM0
      vm0 = jupiter->create_vm("vm0", 1);
      vm0->start();

      daemon = vm0->add_actor("actor_daemon", actor_daemon, std::ref(tasks_done));
      jupiter->add_actor("actor_daemonJUPI", actor_daemon, std::ref(tasks_done));

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
  xbt_assert(argc == 3, "Usage: %s platform_file test_number\n\tExample: %s platform.xml 1\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("test_launcher", test_launcher, std::stoi(argv[2]));

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
