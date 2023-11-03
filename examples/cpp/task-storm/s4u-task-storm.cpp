/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example takes the main concepts of Apache Storm presented here
   https://storm.apache.org/releases/2.4.0/Concepts.html and use them to build a simulation of a stream processing
   application

   Spout SA produces data every 100ms. The volume produced is alternatively 1e3, 1e6 and 1e9 bytes.
   Spout SB produces 1e6 bytes every 200ms.

   Bolt B1 and B2 processes data from Spout SA alternatively. The quantity of work to process this data is 10 flops per
   bytes Bolt B3 processes data from Spout SB. Bolt B4 processes data from Bolt B3.

                        Fafard
                        ┌────┐
                    ┌──►│ B1 │
         Tremblay   │   └────┘
          ┌────┐    │
          │ SA ├────┤  Ginette
          └────┘    │   ┌────┐
                    └──►│ B2 │
                        └────┘


                       Bourassa
         Jupiter     ┌──────────┐
          ┌────┐     │          │
          │ SB ├─────┤ B3 ──► B4│
          └────┘     │          │
                     └──────────┘
 */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_storm, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  // Retrieve hosts
  auto tremblay = e.host_by_name("Tremblay");
  auto jupiter  = e.host_by_name("Jupiter");
  auto fafard   = e.host_by_name("Fafard");
  auto ginette  = e.host_by_name("Ginette");
  auto bourassa = e.host_by_name("Bourassa");

  // Create execution tasks
  auto SA = sg4::ExecTask::init("SA", tremblay->get_speed() * 0.1, tremblay);
  auto SB = sg4::ExecTask::init("SB", jupiter->get_speed() * 0.2, jupiter);
  auto B1 = sg4::ExecTask::init("B1", 1e8, fafard);
  auto B2 = sg4::ExecTask::init("B2", 1e8, ginette);
  auto B3 = sg4::ExecTask::init("B3", 1e8, bourassa);
  auto B4 = sg4::ExecTask::init("B4", 2e8, bourassa);

  // Create communication tasks
  auto SA_to_B1 = sg4::CommTask::init("SA_to_B1", 0, tremblay, fafard);
  auto SA_to_B2 = sg4::CommTask::init("SA_to_B2", 0, tremblay, ginette);
  auto SB_to_B3 = sg4::CommTask::init("SB_to_B3", 1e6, jupiter, bourassa);

  // Create the graph by defining dependencies between tasks
  // Some dependencies are defined dynamically
  SA_to_B1->add_successor(B1);
  SA_to_B2->add_successor(B2);
  SB->add_successor(SB_to_B3);
  SB_to_B3->add_successor(B3);
  B3->add_successor(B4);

  /* Dynamic modification of the graph and bytes sent
     Alternatively we: remove/add the link between SA and SA_to_B2
                       add/remove the link between SA and SA_to_B1
  */
  SA->on_this_completion_cb([&SA_to_B1, &SA_to_B2](sg4::Task* t) {
    int count = t->get_count();
    sg4::CommTaskPtr comm;
    if (count % 2 == 1) {
      t->remove_successor(SA_to_B2);
      t->add_successor(SA_to_B1);
      comm = SA_to_B1;
    } else {
      t->remove_successor(SA_to_B1);
      t->add_successor(SA_to_B2);
      comm = SA_to_B2;
    }
    std::vector<double> amount = {1e9, 1e3, 1e6};
    // XBT_INFO("Comm %f", amount[count % 3]);
    comm->set_amount(amount[count % 3]);
    auto token = std::make_shared<sg4::Token>();
    token->set_data(new double(amount[count % 3]));
    t->set_token(token);
  });

  // The token sent by SA is forwarded by both communication tasks
  SA_to_B1->on_this_completion_cb([&SA](sg4::Task* t) {
    t->set_token(t->get_token_from(SA));
    t->deque_token_from(SA);
  });
  SA_to_B2->on_this_completion_cb([&SA](sg4::Task* t) {
    t->set_token(t->get_token_from(SA));
    t->deque_token_from(SA);
  });

  /* B1 and B2 read the value of the token received by their predecessors
     and use it to adapt their amount of work to do.
  */
  B1->on_this_start_cb([&SA_to_B1](sg4::Task* t) {
    auto data = t->get_token_from(SA_to_B1)->get_data<double>();
    t->deque_token_from(SA_to_B1);
    t->set_amount(*data * 10);
    delete data;
  });
  B2->on_this_start_cb([&SA_to_B2](sg4::Task* t) {
    auto data = t->get_token_from(SA_to_B2)->get_data<double>();
    t->deque_token_from(SA_to_B2);
    t->set_amount(*data * 10);
    delete data;
  });

  // Enqueue firings for tasks without predecessors
  SA->enqueue_firings(5);
  SB->enqueue_firings(5);

  // Add a function to be called when tasks end for log purpose
  sg4::Task::on_completion_cb(
      [](const sg4::Task* t) { XBT_INFO("Task %s finished (%d)", t->get_name().c_str(), t->get_count()); });

  // Start the simulation
  e.run();
  return 0;
}
