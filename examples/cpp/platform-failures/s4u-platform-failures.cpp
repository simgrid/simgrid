/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to work with the state profile of a host or a link,
 * specifying when the resource must be turned on or off.
 *
 * To set such a profile, the first way is to use a file in the XML, while the second is to use the programmatic
 * interface, as exemplified in the main() below. Once this profile is in place, the resource will automatically
 * be turned on and off.
 *
 * The actors running on a host that is turned off are forcefully killed
 * once their on_exit callbacks are executed. They cannot avoid this fate.
 * Since we specified on_failure="RESTART" for each actors in the XML file,
 * they will be automatically restarted when the host starts again.
 *
 * Communications using failed links will .. fail.
 */

#include "simgrid/kernel/ProfileBuilder.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void master(std::vector<std::string> args)
{
  xbt_assert(args.size() == 5, "Expecting one parameter");

  simgrid::s4u::Mailbox* mailbox;
  long number_of_tasks = std::stol(args[1]);
  double comp_size     = std::stod(args[2]);
  long comm_size       = std::stol(args[3]);
  long workers_count   = std::stol(args[4]);

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);

  for (int i = 0; i < number_of_tasks; i++) {
    mailbox         = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(i % workers_count));
    auto* payload   = new double(comp_size);
    try {
      XBT_INFO("Send a message to %s", mailbox->get_cname());
      mailbox->put(payload, comm_size, 10.0);
      XBT_INFO("Send to %s completed", mailbox->get_cname());
    } catch (const simgrid::TimeoutException&) {
      delete payload;
      XBT_INFO("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!", mailbox->get_cname());
    } catch (const simgrid::NetworkFailureException&) {
      delete payload;
      XBT_INFO("Mmh. The communication with '%s' failed. Nevermind. Let's keep going!", mailbox->get_cname());
    }
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (int i = 0; i < workers_count; i++) {
    /* - Eventually tell all the workers to stop by sending a "finalize" task */
    mailbox         = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(i));
    auto* payload   = new double(-1.0);
    try {
      mailbox->put(payload, 0, 1.0);
    } catch (const simgrid::TimeoutException&) {
      delete payload;
      XBT_INFO("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!", mailbox->get_cname());
    } catch (const simgrid::NetworkFailureException&) {
      delete payload;
      XBT_INFO("Mmh. Something went wrong with '%s'. Nevermind. Let's keep going!", mailbox->get_cname());
    }
  }

  XBT_INFO("Goodbye now!");
}

static void worker(std::vector<std::string> args)
{
  xbt_assert(args.size() == 2, "Expecting one parameter");
  long id                          = std::stol(args[1]);
  simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(id));
  while (true) {
    try {
      XBT_INFO("Waiting a message on %s", mailbox->get_cname());
      auto payload = mailbox->get_unique<double>();
      xbt_assert(payload != nullptr, "mailbox->get() failed");
      double comp_size = *payload;
      if (comp_size < 0) { /* - Exit when -1.0 is received */
        XBT_INFO("I'm done. See you!");
        break;
      }
      /*  - Otherwise, process the task */
      XBT_INFO("Start execution...");
      simgrid::s4u::this_actor::execute(comp_size);
      XBT_INFO("Execution complete.");
    } catch (const simgrid::NetworkFailureException&) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    }
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  // This is how to attach a profile to an host that is created from the XML file.
  // This should be done before calling load_platform(), as the on_creation() event is fired when loading the platform.
  // You can never set a new profile to a resource that already have one.
  simgrid::s4u::Host::on_creation.connect([](simgrid::s4u::Host& h) {
    if (h.get_name() == "Bourrassa") {
      h.set_state_profile(simgrid::kernel::profile::ProfileBuilder::from_string("bourassa_profile", "67 0\n70 1\n", 0));
    }
  });
  e.load_platform(argv[1]);

  e.register_function("master", master);
  e.register_function("worker", worker);
  e.load_deployment(argv[2]);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());
  return 0;
}
