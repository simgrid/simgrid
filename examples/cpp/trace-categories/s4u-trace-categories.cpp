/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This source code simply loads the platform. This is only useful to play
 * with the tracing module. See the tesh file to see how to generate the
 * traces.
 */

#include "simgrid/instr.h"
#include "simgrid/s4u.hpp"

struct Task {
  std::string name;
  std::string category;
  double flops;
  uint64_t bytes;
};

static void master()
{
  auto mbox = simgrid::s4u::Mailbox::by_name("master_mailbox");
  for (int i = 0; i < 10; i++) {
    Task task;
    if (i % 2)
      task = {"task_compute", "compute", 10000000, 0};
    else if (i % 3)
      task = {"task_request", "request", 10, 10};
    else
      task = {"task_data", "data", 10, 10000000};
    mbox->put(new Task(task), task.bytes);
  }
  Task finalize = {"finalize", "finalize", 0, 1000};
  mbox->put(new Task(finalize), finalize.bytes);
}

static void worker()
{
  auto mbox = simgrid::s4u::Mailbox::by_name("master_mailbox");
  while (true) {
    auto task = mbox->get_unique<Task>();
    if (task->name == "finalize") {
      break;
    }
    // creating task and setting its category
    simgrid::s4u::this_actor::exec_init(task->flops)
        ->set_name(task->name)
        ->set_tracing_category(task->category)
        ->wait();
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n \tExample: %s small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  // declaring user categories with RGB colors
  simgrid::instr::declare_tracing_category("compute", "1 0 0");  // red
  simgrid::instr::declare_tracing_category("request", "0 1 0");  // green
  simgrid::instr::declare_tracing_category("data", "0 0 1");     // blue
  simgrid::instr::declare_tracing_category("finalize", "0 0 0"); // black

  simgrid::s4u::Actor::create("master", e.host_by_name("Tremblay"), master);
  simgrid::s4u::Actor::create("worker", e.host_by_name("Fafard"), worker);

  e.run();
  return 0;
}
