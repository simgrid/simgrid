/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "xbt/sysdep.h"
#include <simgrid/s4u.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_masterworker, "Messages specific for this s4u example");

class Master {
  long number_of_tasks             = 0; /* - Number of tasks      */
  double comp_size                 = 0; /* - Task compute cost    */
  double comm_size                 = 0; /* - Task communication size */
  long workers_count               = 0; /* - Number of workers    */
  simgrid::s4u::MailboxPtr mailbox = nullptr;

public:
  explicit Master(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 5, "The master function expects 4 arguments from the XML deployment file");

    number_of_tasks = xbt_str_parse_int(args[1].c_str(), "Invalid amount of tasks: %s"); /* - Number of tasks */
    comp_size       = xbt_str_parse_double(args[2].c_str(), "Invalid computational size: %s"); /* - Task compute cost */
    comm_size       = xbt_str_parse_double(args[3].c_str(), "Invalid communication size: %s"); /* - Communication size */
    workers_count   = xbt_str_parse_int(args[4  ].c_str(), "Invalid amount of workers: %s"); /* - Number of workers */

    XBT_INFO("Got %ld workers and %ld tasks to process", workers_count, number_of_tasks);
  }

  void operator()()
  {
    for (int i = 0; i < number_of_tasks; i++) { /* For each task to be executed: */
      /* - Select a @ref worker in a round-robin way */
      mailbox = simgrid::s4u::Mailbox::byName(std::string("worker-") + std::to_string(i % workers_count));

      if (number_of_tasks < 10000 || i % 10000 == 0)
        XBT_INFO("Sending \"%s\" (of %ld) to mailbox \"%s\"", (std::string("Task_") + std::to_string(i)).c_str(),
                                                              number_of_tasks, mailbox->name());

      /* - Send the task to the @ref worker */
      char* payload = bprintf("%f", comp_size);
      simgrid::s4u::this_actor::send(mailbox, payload, comm_size);
    }

    XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
    for (int i = 0; i < workers_count; i++) {
      /* - Eventually tell all the workers to stop by sending a "finalize" task */
      mailbox = simgrid::s4u::Mailbox::byName(std::string("worker-") + std::to_string(i % workers_count));
      simgrid::s4u::this_actor::send(mailbox, xbt_strdup("finalize"), 0);
    }
  }
};

class Worker {
  long id                          = -1;
  simgrid::s4u::MailboxPtr mailbox = nullptr;

public:
  explicit Worker(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 2, "The worker expects a single argument from the XML deployment file: "
                                 "its worker ID (its numerical rank)");
    id      = xbt_str_parse_int(args[1].c_str(), "Invalid argument %s");
    mailbox = simgrid::s4u::Mailbox::byName(std::string("worker-") + std::to_string(id));
  }

  void operator()()
  {
    while (1) { /* The worker waits in an infinite loop for tasks sent by the \ref master */
      char* res = static_cast<char*>(simgrid::s4u::this_actor::recv(mailbox));
      xbt_assert(res != nullptr, "MSG_task_get failed");

      if (strcmp(res, "finalize") == 0) { /* - Exit if 'finalize' is received */
        xbt_free(res);
        break;
      }
      /*  - Otherwise, process the task */
      double comp_size = xbt_str_parse_double(res, nullptr);
      xbt_free(res);
      simgrid::s4u::this_actor::execute(comp_size);
    }
    XBT_INFO("I'm done. See you!");
  }
};

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  e->loadPlatform(argv[1]);              /** - Load the platform description */
  e->registerFunction<Master>("master");
  e->registerFunction<Worker>("worker"); /** - Register the function to be executed by the processes */
  e->loadDeployment(argv[2]);            /** - Deploy the application */

  e->run(); /** - Run the simulation */

  XBT_INFO("Simulation time %g", e->getClock());

  return 0;
}
