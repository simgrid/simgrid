/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define CATCH_CONFIG_RUNNER // we supply our own main()

#include "activity-lifecycle.hpp"

#include <xbt/config.hpp>

XBT_LOG_NEW_CATEGORY(s4u_test, "Messages specific for this s4u example");

std::vector<simgrid::s4u::Host*> all_hosts;

/* Helper function easing the testing of actor's ending condition */
void assert_exit(bool exp_success, double duration)
{
  double expected_time = simgrid::s4u::Engine::get_clock() + duration;
  simgrid::s4u::this_actor::on_exit([exp_success, expected_time](bool got_failed) {
    XBT_VERB("Running checks on exit");
    INFO("Check exit status. Expected: " << exp_success);
    REQUIRE(exp_success == not got_failed);
    INFO("Check date at exit. Expected: " + std::to_string(expected_time));
    REQUIRE(simgrid::s4u::Engine::get_clock() == Approx(expected_time));
    XBT_VERB("Checks on exit successful");
  });
}

/* Helper function in charge of doing some sanity checks after each test */
void assert_cleanup()
{
  /* Check that no actor remain (but on host[0], where main_dispatcher lives */
  for (unsigned int i = 0; i < all_hosts.size(); i++) {
    std::vector<simgrid::s4u::ActorPtr> all_actors = all_hosts[i]->get_all_actors();
    unsigned int expected_count = (i == 0) ? 1 : 0; // host[0] contains main_dispatcher, all other are empty
    if (all_actors.size() != expected_count) {
      INFO("Host " << all_hosts[i]->get_cname() << " contains " << all_actors.size() << " actors but " << expected_count
                   << " are expected (i=" << i << "). Existing actors: ");
      for (auto act : all_actors)
        UNSCOPED_INFO(" - " << act->get_cname());
      FAIL("This is wrong");
    }
  }
  // TODO: Check that all LMM are empty
}

int main(int argc, char* argv[])
{
  simgrid::config::set_value("help-nostop", true);
  simgrid::s4u::Engine e(&argc, argv);

  std::string platf;
  if (argc > 1) {
    platf   = argv[1];
    argv[1] = argv[0];
    argv++;
    argc--;
  } else {
    XBT_WARN("No platform file provided. Using './testing_platform.xml'");
    platf = "./testing_platform.xml";
  }
  e.load_platform(platf);

  int status = 42;
  all_hosts = e.get_all_hosts();
  simgrid::s4u::Actor::create("main_dispatcher", all_hosts[0],
                              [&argc, &argv, &status]() { status = Catch::Session().run(argc, argv); });

  e.run();
  XBT_INFO("Simulation done");
  return status;
}
