/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to work with the state profile of an host or a link,
 * specifying when the resource must be turned on or off.
 *
 * To set such a profile, the first way is to use a file in the XML, while the second is to use the programmatic
 * interface. Once this profile is in place, the resource will automatically be turned on and off.
 *
 * The actors running on an host that is turned off are forcefully killed
 * once their on_exit callbacks are executed. They cannot avoid this fate.
 * Since we specified on_failure="RESTART" for each actors in the XML file,
 * they will be automatically restarted when the host starts again.
 *
 * Communications using failed links will .. fail.
 */

#include "simgrid/s4u.hpp"
#include "xbt/str.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static int master(int argc, char* argv[])
{
  xbt_assert(argc == 5, "Expecting one parameter");

  simgrid::s4u::Mailbox* mailbox;
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double comp_size     = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double comm_size     = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long workers_count   = xbt_str_parse_int(argv[4], "Invalid amount of workers: %s");

  XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);

  for (int i = 0; i < number_of_tasks; i++) {
    mailbox         = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(i % workers_count));
    double* payload = new double(comp_size);
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
    double* payload = new double(-1.0);
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
  return 0;
}

static int worker(int argc, char* argv[])
{
  xbt_assert(argc == 2, "Expecting one parameter");
  long id                          = xbt_str_parse_int(argv[1], "Invalid argument %s");
  simgrid::s4u::Mailbox* mailbox   = simgrid::s4u::Mailbox::by_name(std::string("worker-") + std::to_string(id));
  const double* payload            = nullptr;
  double comp_size                 = -1;
  while (1) {
    try {
      XBT_INFO("Waiting a message on %s", mailbox->get_cname());
      payload   = static_cast<double*>(mailbox->get());
      xbt_assert(payload != nullptr, "mailbox->get() failed");
      comp_size = *payload;
      delete payload;
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
  return 0;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  e.register_function("master", master);
  e.register_function("worker", worker);
  e.load_deployment(argv[2]);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());
  return 0;
}
