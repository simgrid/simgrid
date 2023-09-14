/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example illustrates how to use Simgrid Tasks to reproduce the workflow 2.a of the article
 * "Automated performance prediction of microservice applications using simulation" by Clément Courageux-Sudan et al.
 *
 * To build this workflow we create;
 *   - each Execution Task
 *   - each Communication Task
 *   - the links between the Tasks, i.e. the graph
 *
 * We also increase the parallelism degree of each Task to 10.
 *
 * In this scenario we send 500 requests per second (RPS) to the entry point of the graph for 7 seconds,
 * and we count the number of processed requests between 2 and 7 seconds to evaluate the number of requests processed
 * per second.
 */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(task_microservice, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void request_sender(sg4::TaskPtr t, int requests_per_second)
{
  for (int i = 0; i < requests_per_second * 7; i++) {
    t->enqueue_firings(1);
    sg4::this_actor::sleep_for(1.0 / requests_per_second);
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  // Retrieve Hosts
  auto pm0 = e.host_by_name("PM0");
  auto pm1 = e.host_by_name("PM1");

  // Set concurrency limit
  pm0->set_concurrency_limit(10);
  pm1->set_concurrency_limit(10);

  // Create Exec Tasks
  auto nginx_web_server        = sg4::ExecTask::init("nginx_web_server", 783 * pm0->get_speed() / 1e6, pm0);
  auto compose_post_service_0  = sg4::ExecTask::init("compose_post_service_0", 682 * pm0->get_speed() / 1e6, pm0);
  auto unique_id_service       = sg4::ExecTask::init("unique_id_service", 12 * pm0->get_speed() / 1e6, pm0);
  auto compose_post_service_1  = sg4::ExecTask::init("compose_post_service_1", 140 * pm0->get_speed() / 1e6, pm0);
  auto media_service           = sg4::ExecTask::init("media_service", 6 * pm1->get_speed() / 1e6, pm1);
  auto compose_post_service_2  = sg4::ExecTask::init("compose_post_service_2", 135 * pm0->get_speed() / 1e6, pm0);
  auto user_service            = sg4::ExecTask::init("user_service", 5 * pm0->get_speed() / 1e6, pm0);
  auto compose_post_service_3  = sg4::ExecTask::init("compose_post_service_3", 147 * pm0->get_speed() / 1e6, pm0);
  auto text_service_0          = sg4::ExecTask::init("text_service_0", 296 * pm0->get_speed() / 1e6, pm0);
  auto text_service_1          = sg4::ExecTask::init("text_service_1", 350 * pm0->get_speed() / 1e6, pm0);
  auto text_service_2          = sg4::ExecTask::init("text_service_2", 146 * pm0->get_speed() / 1e6, pm0);
  auto user_mention_service    = sg4::ExecTask::init("user_mention_service", 934 * pm0->get_speed() / 1e6, pm0);
  auto url_shorten_service     = sg4::ExecTask::init("url_shorten_service", 555 * pm0->get_speed() / 1e6, pm0);
  auto compose_post_service_4  = sg4::ExecTask::init("compose_post_service_4", 138 * pm0->get_speed() / 1e6, pm0);
  auto home_timeline_service_0 = sg4::ExecTask::init("home_timeline_service_0", 243 * pm0->get_speed() / 1e6, pm0);
  auto social_graph_service    = sg4::ExecTask::init("home_timeline_service_0", 707 * pm0->get_speed() / 1e6, pm0);
  auto home_timeline_service_1 = sg4::ExecTask::init("home_timeline_service_0", 7 * pm0->get_speed() / 1e6, pm0);
  auto compose_post_service_5  = sg4::ExecTask::init("compose_post_service_5", 192 * pm0->get_speed() / 1e6, pm0);
  auto user_timeline_service   = sg4::ExecTask::init("user_timeline_service", 913 * pm0->get_speed() / 1e6, pm0);
  auto compose_post_service_6  = sg4::ExecTask::init("compose_post_service_6", 508 * pm0->get_speed() / 1e6, pm0);
  auto post_storage_service    = sg4::ExecTask::init("post_storage_service", 391 * pm1->get_speed() / 1e6, pm1);

  // Create Comm Tasks
  auto compose_post_service_1_to_media_service =
      sg4::CommTask::init("compose_post_service_1_to_media_service", 100, pm0, pm1);
  auto media_service_to_compose_post_service_2 =
      sg4::CommTask::init("media_service_to_compose_post_service_2", 100, pm1, pm0);
  auto compose_post_service_6_to_post_storage_service =
      sg4::CommTask::init("media_service_to_compose_post_service_2", 100, pm1, pm0);

  // Create the graph
  nginx_web_server->add_successor(compose_post_service_0);
  compose_post_service_0->add_successor(unique_id_service);
  unique_id_service->add_successor(compose_post_service_1);
  compose_post_service_1->add_successor(compose_post_service_1_to_media_service);
  compose_post_service_1_to_media_service->add_successor(media_service);
  media_service->add_successor(media_service_to_compose_post_service_2);
  media_service_to_compose_post_service_2->add_successor(compose_post_service_2);
  compose_post_service_2->add_successor(user_service);
  user_service->add_successor(compose_post_service_3);
  compose_post_service_3->add_successor(text_service_0);
  text_service_0->add_successor(text_service_1);
  text_service_0->add_successor(text_service_2);
  text_service_1->add_successor(user_mention_service);
  text_service_2->add_successor(url_shorten_service);
  user_mention_service->add_successor(compose_post_service_4);
  compose_post_service_4->add_successor(home_timeline_service_0);
  home_timeline_service_0->add_successor(social_graph_service);
  social_graph_service->add_successor(home_timeline_service_1);
  home_timeline_service_1->add_successor(compose_post_service_5);
  compose_post_service_5->add_successor(user_timeline_service);
  user_timeline_service->add_successor(compose_post_service_6);
  compose_post_service_6->add_successor(compose_post_service_6_to_post_storage_service);
  compose_post_service_6_to_post_storage_service->add_successor(post_storage_service);

  // Dispatch Exec Tasks
  std::vector<sg4::TaskPtr> exec_tasks = {nginx_web_server,
                                          compose_post_service_0,
                                          unique_id_service,
                                          compose_post_service_1,
                                          compose_post_service_1_to_media_service,
                                          media_service,
                                          media_service_to_compose_post_service_2,
                                          compose_post_service_2,
                                          user_service,
                                          compose_post_service_3,
                                          text_service_0,
                                          text_service_1,
                                          text_service_2,
                                          user_mention_service,
                                          url_shorten_service,
                                          compose_post_service_4,
                                          home_timeline_service_0,
                                          social_graph_service,
                                          home_timeline_service_1,
                                          compose_post_service_5,
                                          user_timeline_service,
                                          compose_post_service_6,
                                          compose_post_service_6_to_post_storage_service,
                                          post_storage_service};
  for (auto t : exec_tasks)
    t->set_parallelism_degree(10);

  // Create the actor that will inject requests during the simulation
  sg4::Actor::create("request_sender", pm0, request_sender, nginx_web_server, 500);

  // Add a function to be called when tasks end for log purpose
  int requests_processed = 0;
  sg4::Task::on_completion_cb([&e, &requests_processed](const sg4::Task* t) {
    if (t->get_name() == "post_storage_service" and e.get_clock() < 7 and e.get_clock() > 2)
      requests_processed++;
  });

  // Start the simulation
  e.run();
  XBT_INFO("Requests processed per second: %f", requests_processed / 5.0);
  return 0;
}
