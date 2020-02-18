/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define CATCH_CONFIG_RUNNER // we supply our own main()

#include "../../src/include/catch.hpp"

#include "simgrid/s4u.hpp"
#include <xbt/config.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

// FIXME: fix failing tests, and remove this macro
#define FAILING if (false)

std::vector<simgrid::s4u::Host*> all_hosts;

/* Helper function easing the testing of actor's ending condition */
static void assert_exit(bool exp_success, double duration)
{
  double expected_time = simgrid::s4u::Engine::get_clock() + duration;
  simgrid::s4u::this_actor::on_exit([exp_success, expected_time](bool got_failed) {
    INFO("Check exit status. Expected: " << exp_success);
    REQUIRE(exp_success == not got_failed);
    INFO("Check date at exit. Expected: " << expected_time);
    REQUIRE(simgrid::s4u::Engine::get_clock() == Approx(expected_time));
    XBT_VERB("Checks on exit successful");
  });
}

/* Helper function in charge of doing some sanity checks after each test */
static void assert_cleanup()
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

/**
 ** Each tests
 **/

//========== Creators: create an async activity

// Create a new async execution with given duration
static simgrid::s4u::ActivityPtr create_exec(double duration)
{
  double speed = simgrid::s4u::this_actor::get_host()->get_speed();
  return simgrid::s4u::this_actor::exec_async(speed * duration);
}

// TODO: check other kinds of activities too (Io, Comm, ...)

using creator_type = decltype(create_exec);

//========== Testers: test the completion of an activity

// Calls exec->test() and returns its result
static bool tester_test(const simgrid::s4u::ActivityPtr& exec)
{
  return exec->test();
}

// Calls exec->wait_for(Duration * 0.0125) and returns true when exec is terminated, just like test()
template <int Duration> bool tester_wait(const simgrid::s4u::ActivityPtr& exec)
{
  bool ret;
  try {
    exec->wait_for(Duration * 0.0125);
    XBT_DEBUG("wait_for() returned normally");
    ret = true;
  } catch (const simgrid::TimeoutException& e) {
    XBT_DEBUG("wait_for() timed out (%s)", e.what());
    ret = false;
  } catch (const simgrid::Exception& e) {
    XBT_DEBUG("wait_for() threw an exception: %s", e.what());
    ret = true;
  }
  return ret;
}

using tester_type = decltype(tester_test);

//========== Waiters: wait for the completion of an activity

// Wait for 6s
static void waiter_sleep6(const simgrid::s4u::ActivityPtr&)
{
  simgrid::s4u::this_actor::sleep_for(6.0);
  XBT_DEBUG("wake up after 6s sleep");
}

// Wait for completion of exec
static void waiter_wait(const simgrid::s4u::ActivityPtr& exec)
{
  exec->wait();
  XBT_DEBUG("end of wait()");
}

using waiter_type = decltype(waiter_wait);

//========== Finally, the test templates

template <creator_type Create, tester_type Test> void test_trivial()
{
  XBT_INFO("Launch an activity for 5s, and let it proceed before test");

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], []() {
    assert_exit(true, 6.);
    simgrid::s4u::ActivityPtr activity = Create(5.0);
    simgrid::s4u::this_actor::sleep_for(6.0);
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  exec5->join();
}

template <creator_type Create, tester_type Test> void test_basic()
{
  XBT_INFO("Launch an activity for 5s, and test while it proceeds");

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], []() {
    assert_exit(true, 6.);
    simgrid::s4u::ActivityPtr activity = Create(5.0);
    for (int i = 0; i < 3; i++) {
      INFO("activity should be still running (i = " << i << ")");
      REQUIRE(not Test(activity));
      simgrid::s4u::this_actor::sleep_for(2.0);
    }
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  exec5->join();
}

template <creator_type Create, tester_type Test> void test_cancel()
{
  XBT_INFO("Launch an activity for 5s, and cancel it after 2s");

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], []() {
    assert_exit(true, 2.);
    simgrid::s4u::ActivityPtr activity = Create(5.0);
    simgrid::s4u::this_actor::sleep_for(2.0);
    activity->cancel();
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  exec5->join();
}

template <creator_type Create, tester_type Test, waiter_type Wait> void test_failure_actor()
{
  XBT_INFO("Launch an activity for 5s, and kill running actor after 2s");

  simgrid::s4u::ActivityPtr activity;
  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], [&activity]() {
    assert_exit(false, 2.);
    activity = Create(5.0);
    Wait(activity);
    FAIL("should not be here!");
  });
  simgrid::s4u::this_actor::sleep_for(2.0);
  INFO("activity should be still running");
  REQUIRE(not Test(activity));
  exec5->kill();
  INFO("activity should be terminated now");
  REQUIRE(Test(activity));
}

template <creator_type Create, tester_type Test, waiter_type Wait> void test_failure_host()
{
  XBT_INFO("Launch an activity for 5s, and shutdown host 2s");

  simgrid::s4u::ActivityPtr activity;
  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], [&activity]() {
    assert_exit(false, 2.);
    activity = Create(5.0);
    Wait(activity);
    FAIL("should not be here!");
  });
  simgrid::s4u::this_actor::sleep_for(2.0);
  INFO("activity should be still running");
  REQUIRE(not Test(activity));
  exec5->get_host()->turn_off();
  exec5->get_host()->turn_on();
  INFO("activity should be terminated now");
  REQUIRE(Test(activity));
}

//==========

/* We need an extra actor here, so that it can sleep until the end of each test */
#define RUN_SECTION(descr, ...) SECTION(descr) { simgrid::s4u::Actor::create(descr, all_hosts[0], __VA_ARGS__); }

TEST_CASE("Activity test/wait: using <tester_test>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and test once", test_trivial<create_exec, tester_test>);
  RUN_SECTION("exec: run and test many", test_basic<create_exec, tester_test>);
  RUN_SECTION("exec: cancel and test", test_cancel<create_exec, tester_test>);
  FAILING{} RUN_SECTION("exec: actor failure and test / sleep", test_failure_actor<create_exec, tester_test, waiter_sleep6>);
  FAILING RUN_SECTION("exec: host failure and test / sleep", test_failure_host<create_exec, tester_test, waiter_sleep6>);
  RUN_SECTION("exec: actor failure and test / wait", test_failure_actor<create_exec, tester_test, waiter_wait>);
  RUN_SECTION("exec: host failure and test / wait", test_failure_host<create_exec, tester_test, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait<0>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait<0> once", test_trivial<create_exec, tester_wait<0>>);
  FAILING RUN_SECTION("exec: run and wait<0> many", test_basic<create_exec, tester_wait<0>>);
  RUN_SECTION("exec: cancel and wait<0>", test_cancel<create_exec, tester_wait<0>>);
  FAILING RUN_SECTION("exec: actor failure and wait<0> / sleep", test_failure_actor<create_exec, tester_wait<0>, waiter_sleep6>);
  FAILING RUN_SECTION("exec: host failure and wait<0> / sleep", test_failure_host<create_exec, tester_wait<0>, waiter_sleep6>);
  FAILING RUN_SECTION("exec: actor failure and wait<0> / wait", test_failure_actor<create_exec, tester_wait<0>, waiter_wait>);
  FAILING RUN_SECTION("exec: host failure and wait<0> / wait", test_failure_host<create_exec, tester_wait<0>, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait<1>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait<1> once", test_trivial<create_exec, tester_wait<1>>);
  FAILING RUN_SECTION("exec: run and wait<1> many", test_basic<create_exec, tester_wait<1>>);
  RUN_SECTION("exec: cancel and wait<1>", test_cancel<create_exec, tester_wait<1>>);
  FAILING RUN_SECTION("exec: actor failure and wait<1> / sleep", test_failure_actor<create_exec, tester_wait<1>, waiter_sleep6>);
  FAILING RUN_SECTION("exec: host failure and wait<1> / sleep", test_failure_host<create_exec, tester_wait<1>, waiter_sleep6>);
  FAILING RUN_SECTION("exec: actor failure and wait<1> / wait", test_failure_actor<create_exec, tester_wait<1>, waiter_wait>);
  FAILING RUN_SECTION("exec: host failure and wait<1> / wait", test_failure_host<create_exec, tester_wait<1>, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
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
