/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

// FIXME: fix failing tests, and remove this macro
#define FAILING if (false)

std::vector<simgrid::s4u::Host*> all_hosts;

/* Helper function easing the testing of actor's ending condition */
static void assert_exit(bool exp_failed, double duration)
{
  double expected_time = simgrid::s4u::Engine::get_clock() + duration;
  simgrid::s4u::this_actor::on_exit([exp_failed, expected_time](bool got_failed) {
    xbt_assert(exp_failed == got_failed, "Exit failure status mismatch. Expected %d, got %d", exp_failed, got_failed);
    xbt_assert(std::fabs(expected_time - simgrid::s4u::Engine::get_clock()) < 0.001, "Exit time mismatch. Expected %f",
               expected_time);
    XBT_VERB("Checks on exit successful");
  });
}
/* Helper function in charge of running a test and doing some sanity checks afterward */
static void run_test(const char* test_name, const std::function<void()>& test)
{
  simgrid::s4u::Actor::create(test_name, all_hosts[0], test);
  simgrid::s4u::this_actor::sleep_for(10);

  /* Check that no actor remain (but on host[0], where main_dispatcher lives */
  for (unsigned int i = 0; i < all_hosts.size(); i++) {
    std::vector<simgrid::s4u::ActorPtr> all_actors = all_hosts[i]->get_all_actors();
    unsigned int expected_count = (i == 0) ? 1 : 0; // host[0] contains main_dispatcher, all other are empty
    if (all_actors.size() != expected_count) {
      XBT_CRITICAL("Host %s contains %zu actors but %u are expected (i=%u). Existing actors: ",
                   all_hosts[i]->get_cname(), all_actors.size(), expected_count, i);
      for (auto act : all_actors)
        XBT_CRITICAL(" - %s", act->get_cname());
      xbt_die("This is wrong");
    }
  }
  xbt_assert("TODO: Check that all LMM are empty");
  XBT_INFO("SUCCESS: %s", test_name);
  XBT_INFO("#########################################################################################################");
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
  XBT_INFO("%s: Launch an activity for 5s, and let it proceed before test", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], []() {
    assert_exit(false, 6.);
    simgrid::s4u::ActivityPtr activity = Create(5.0);
    simgrid::s4u::this_actor::sleep_for(6.0);
    xbt_assert(Test(activity), "activity should be terminated now");
  });
  exec5->join();
}

template <creator_type Create, tester_type Test> void test_basic()
{
  XBT_INFO("%s: Launch an activity for 5s, and test while it proceeds", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], []() {
    assert_exit(false, 6.);
    simgrid::s4u::ActivityPtr activity = Create(5.0);
    for (int i = 0; i < 3; i++) {
      xbt_assert(not Test(activity), "activity finished too soon (i = %d)!?", i);
      simgrid::s4u::this_actor::sleep_for(2.0);
    }
    xbt_assert(Test(activity), "activity should be terminated now");
  });
  exec5->join();
}

template <creator_type Create, tester_type Test> void test_cancel()
{
  XBT_INFO("%s: Launch an activity for 5s, and cancel it after 2s", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], []() {
    assert_exit(false, 2.);
    simgrid::s4u::ActivityPtr activity = Create(5.0);
    simgrid::s4u::this_actor::sleep_for(2.0);
    activity->cancel();
    xbt_assert(Test(activity), "activity should be terminated now");
  });
  exec5->join();
}

template <creator_type Create, tester_type Test, waiter_type Wait> void test_failure_actor()
{
  XBT_INFO("%s: Launch an activity for 5s, and kill running actor after 2s", __func__);

  simgrid::s4u::ActivityPtr activity;
  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], [&activity]() {
    assert_exit(true, 2.);
    activity = Create(5.0);
    Wait(activity);
    xbt_die("should not be here!");
  });
  simgrid::s4u::this_actor::sleep_for(2.0);
  xbt_assert(not Test(activity), "activity finished too soon!?");
  exec5->kill();
  xbt_assert(Test(activity), "activity should be terminated now");
}

template <creator_type Create, tester_type Test, waiter_type Wait> void test_failure_host()
{
  XBT_INFO("%s: Launch an activity for 5s, and shutdown host 2s", __func__);

  simgrid::s4u::ActivityPtr activity;
  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], [&activity]() {
    assert_exit(true, 2.);
    activity = Create(5.0);
    Wait(activity);
    xbt_die("should not be here!");
  });
  simgrid::s4u::this_actor::sleep_for(2.0);
  xbt_assert(not Test(activity), "activity finished too soon!?");
  exec5->get_host()->turn_off();
  exec5->get_host()->turn_on();
  xbt_assert(Test(activity), "activity should be terminated now");
}

//==========

/* We need an extra actor here, so that it can sleep until the end of each test */
static void main_dispatcher()
{
  XBT_INFO("***** Using <tester_test> *****");
  run_test("exec: run and test once", test_trivial<create_exec, tester_test>);
  run_test("exec: run and test many", test_basic<create_exec, tester_test>);
  run_test("exec: cancel and test", test_cancel<create_exec, tester_test>);
  FAILING run_test("exec: actor failure and test / sleep", test_failure_actor<create_exec, tester_test, waiter_sleep6>);
  FAILING run_test("exec: host failure and test / sleep", test_failure_host<create_exec, tester_test, waiter_sleep6>);
  run_test("exec: actor failure and test / wait", test_failure_actor<create_exec, tester_test, waiter_wait>);
  run_test("exec: host failure and test / wait", test_failure_host<create_exec, tester_test, waiter_wait>);

  XBT_INFO("***** Using <tester_wait<0>> *****");
  run_test("exec: run and wait<0> once", test_trivial<create_exec, tester_wait<0>>);
  FAILING run_test("exec: run and wait<0> many", test_basic<create_exec, tester_wait<0>>);
  run_test("exec: cancel and wait<0>", test_cancel<create_exec, tester_wait<0>>);
  FAILING run_test("exec: actor failure and wait<0> / sleep", test_failure_actor<create_exec, tester_wait<0>, waiter_sleep6>);
  FAILING run_test("exec: host failure and wait<0> / sleep", test_failure_host<create_exec, tester_wait<0>, waiter_sleep6>);
  FAILING run_test("exec: actor failure and wait<0> / wait", test_failure_actor<create_exec, tester_wait<0>, waiter_wait>);
  FAILING run_test("exec: host failure and wait<0> / wait", test_failure_host<create_exec, tester_wait<0>, waiter_wait>);

  XBT_INFO("***** Using <tester_wait<1>> *****");
  run_test("exec: run and wait<1> once", test_trivial<create_exec, tester_wait<1>>);
  FAILING run_test("exec: run and wait<1> many", test_basic<create_exec, tester_wait<1>>);
  run_test("exec: cancel and wait<1>", test_cancel<create_exec, tester_wait<1>>);
  FAILING run_test("exec: actor failure and wait<1> / sleep", test_failure_actor<create_exec, tester_wait<1>, waiter_sleep6>);
  FAILING run_test("exec: host failure and wait<1> / sleep", test_failure_host<create_exec, tester_wait<1>, waiter_sleep6>);
  FAILING run_test("exec: actor failure and wait<1> / wait", test_failure_actor<create_exec, tester_wait<1>, waiter_wait>);
  FAILING run_test("exec: host failure and wait<1> / wait", test_failure_host<create_exec, tester_wait<1>, waiter_wait>);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  const char* platf = argv[1];
  if (argc <= 1) {
    XBT_WARN("No platform file provided. Using './testing_platform.xml'");
    platf = "./testing_platform.xml";
  }
  e.load_platform(platf);

  all_hosts = e.get_all_hosts();
  simgrid::s4u::Actor::create("main_dispatcher", all_hosts[0], main_dispatcher);

  e.run();

  XBT_INFO("Simulation done");

  return 0;
}
