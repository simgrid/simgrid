/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "activity-lifecycle.hpp"

//========== Creators: create an async activity

template <typename Activity> using creator_type = Activity (*)(double);

// Create a new async execution with given duration
static simgrid::s4u::ExecPtr create_exec(double duration)
{
  double speed = simgrid::s4u::this_actor::get_host()->get_speed();
  return simgrid::s4u::this_actor::exec_async(speed * duration);
}

// TODO: check other kinds of activities too (Io, Comm, ...)

//========== Testers: test the completion of an activity

template <typename Activity> using tester_type = bool (*)(const Activity&);

// Calls activity->test() and returns its result
template <typename Activity> bool tester_test(const Activity& activity)
{
  return activity->test();
}

// Calls activity->wait_for(Duration / 128.0) and returns true when activity is terminated, just like test()
template <int Duration, typename Activity> bool tester_wait(const Activity& activity)
{
  constexpr double duration = Duration / 128.0;
  const double timeout      = simgrid::s4u::Engine::get_clock() + duration;
  bool ret;
  try {
    XBT_DEBUG("calling wait_for(%f)", duration);
    activity->wait_for(duration);
    XBT_DEBUG("wait_for() returned normally");
    ret = true;
  } catch (const simgrid::TimeoutException& e) {
    XBT_DEBUG("wait_for() timed out (%s)", e.what());
    INFO("wait_for() timeout should expire at expected date: " << timeout);
    REQUIRE(simgrid::s4u::Engine::get_clock() == Approx(timeout));
    ret = false;
  } catch (const simgrid::Exception& e) {
    XBT_DEBUG("wait_for() threw an exception: %s", e.what());
    ret = true;
  }
  INFO("wait_for() should return before timeout expiration at date: " << timeout);
  REQUIRE(simgrid::s4u::Engine::get_clock() <= Approx(timeout));
  return ret;
}

// Calls wait_any_for([activity], Duration / 128.0) and returns true when activity is terminated, just like test()
template <int Duration, typename Activity> bool tester_wait_any(const Activity& activity)
{
  constexpr double duration = Duration / 128.0;
  const double timeout      = simgrid::s4u::Engine::get_clock() + duration;
  bool ret;
  try {
    std::vector<Activity> activities = {activity};
    XBT_DEBUG("calling wait_any_for(%f)", duration);
    int index = Activity::element_type::wait_any_for(&activities, duration);
    if (index == -1) {
      XBT_DEBUG("wait_any_for() timed out");
      INFO("wait_any_for() timeout should expire at expected date: " << timeout);
      REQUIRE(simgrid::s4u::Engine::get_clock() == Approx(timeout));
      ret = false;
    } else {
      XBT_DEBUG("wait_any_for() returned index %d", index);
      REQUIRE(index == 0);
      ret = true;
    }
  } catch (const simgrid::Exception& e) {
    XBT_DEBUG("wait_any_for() threw an exception: %s", e.what());
    ret = true;
  }
  INFO("wait_any_for() should return before timeout expiration at date: " << timeout);
  REQUIRE(simgrid::s4u::Engine::get_clock() <= Approx(timeout));
  return ret;
}

//========== Waiters: wait for the completion of an activity

template <typename Activity> using waiter_type = void (*)(const Activity&);

// Wait for 6s
template <typename Activity> void waiter_sleep6(const Activity&)
{
  simgrid::s4u::this_actor::sleep_for(6.0);
  XBT_DEBUG("wake up after 6s sleep");
}

// Wait for completion of activity
template <typename Activity> void waiter_wait(const Activity& activity)
{
  activity->wait();
  XBT_DEBUG("end of wait()");
}

//========== Finally, the test templates

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test> void test_trivial()
{
  XBT_INFO("Launch an activity for 5s, and let it proceed before test");

  simgrid::s4u::ActorPtr actor = simgrid::s4u::Actor::create("actor", all_hosts[1], []() {
    assert_exit(true, 6.);
    Activity activity = Create(5.0);
    simgrid::s4u::this_actor::sleep_for(6.0);
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  actor->join();
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test> void test_basic()
{
  XBT_INFO("Launch an activity for 5s, and test while it proceeds");

  simgrid::s4u::ActorPtr actor = simgrid::s4u::Actor::create("actor", all_hosts[1], []() {
    assert_exit(true, 6.);
    Activity activity = Create(5.0);
    for (int i = 0; i < 3; i++) {
      const double timestep = simgrid::s4u::Engine::get_clock() + 2.0;
      INFO("activity should be still running (i = " << i << ")");
      REQUIRE(not Test(activity));
      simgrid::s4u::this_actor::sleep_until(timestep);
    }
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  actor->join();
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test> void test_cancel()
{
  XBT_INFO("Launch an activity for 5s, and cancel it after 2s");

  simgrid::s4u::ActorPtr actor = simgrid::s4u::Actor::create("actor", all_hosts[1], []() {
    assert_exit(true, 2.);
    Activity activity = Create(5.0);
    simgrid::s4u::this_actor::sleep_for(2.0);
    activity->cancel();
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  actor->join();
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test, waiter_type<Activity> Wait>
void test_failure_actor()
{
  XBT_INFO("Launch an activity for 5s, and kill running actor after 2s");

  Activity activity;
  simgrid::s4u::ActorPtr actor = simgrid::s4u::Actor::create("actor", all_hosts[1], [&activity]() {
    assert_exit(false, 2.);
    activity = Create(5.0);
    Wait(activity);
    FAIL("should not be here!");
  });
  const double timestep = simgrid::s4u::Engine::get_clock() + 2.0;
  simgrid::s4u::this_actor::sleep_for(1.0);
  INFO("activity should be still running");
  REQUIRE(not Test(activity));
  simgrid::s4u::this_actor::sleep_until(timestep);
  actor->kill();
  INFO("activity should be terminated now");
  REQUIRE(Test(activity));
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test, waiter_type<Activity> Wait>
void test_failure_host()
{
  XBT_INFO("Launch an activity for 5s, and shutdown host 2s");

  Activity activity;
  simgrid::s4u::ActorPtr actor = simgrid::s4u::Actor::create("actor", all_hosts[1], [&activity]() {
    assert_exit(false, 2.);
    activity = Create(5.0);
    Wait(activity);
    FAIL("should not be here!");
  });
  const double timestep = simgrid::s4u::Engine::get_clock() + 2.0;
  simgrid::s4u::this_actor::sleep_for(1.0);
  INFO("activity should be still running");
  REQUIRE(not Test(activity));
  simgrid::s4u::this_actor::sleep_until(timestep);
  actor->get_host()->turn_off();
  actor->get_host()->turn_on();
  INFO("activity should be terminated now");
  REQUIRE(Test(activity));
}

//==========

using simgrid::s4u::ExecPtr;

TEST_CASE("Activity test/wait: using <tester_test>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and test once", test_trivial<ExecPtr, create_exec, tester_test>);
  RUN_SECTION("exec: run and test many", test_basic<ExecPtr, create_exec, tester_test>);
  RUN_SECTION("exec: cancel and test", test_cancel<ExecPtr, create_exec, tester_test>);
  RUN_SECTION("exec: actor failure and test / sleep",
              test_failure_actor<ExecPtr, create_exec, tester_test, waiter_sleep6>);
  RUN_SECTION("exec: host failure and test / sleep",
              test_failure_host<ExecPtr, create_exec, tester_test, waiter_sleep6>);
  RUN_SECTION("exec: actor failure and test / wait",
              test_failure_actor<ExecPtr, create_exec, tester_test, waiter_wait>);
  RUN_SECTION("exec: host failure and test / wait",
              test_failure_host<ExecPtr, create_exec, tester_test, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait<0>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait<0> once", test_trivial<ExecPtr, create_exec, tester_wait<0>>);
  RUN_SECTION("exec: run and wait<0> many", test_basic<ExecPtr, create_exec, tester_wait<0>>);
  RUN_SECTION("exec: cancel and wait<0>", test_cancel<ExecPtr, create_exec, tester_wait<0>>);
  RUN_SECTION("exec: actor failure and wait<0> / sleep",
              test_failure_actor<ExecPtr, create_exec, tester_wait<0>, waiter_sleep6>);
  RUN_SECTION("exec: host failure and wait<0> / sleep",
              test_failure_host<ExecPtr, create_exec, tester_wait<0>, waiter_sleep6>);
  // exec: actor failure and wait<0> / wait
  // exec: host failure and wait<0> / wait

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait<1>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait<1> once", test_trivial<ExecPtr, create_exec, tester_wait<1>>);
  // exec: run and wait<1> many
  RUN_SECTION("exec: cancel and wait<1>", test_cancel<ExecPtr, create_exec, tester_wait<1>>);
  // exec: actor failure and wait<1> / sleep
  // exec: host failure and wait<1> / sleep
  // exec: actor failure and wait<1> / wait
  // exec: host failure and wait<1> / wait

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait_any<0>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait_any<0> once", test_trivial<ExecPtr, create_exec, tester_wait_any<0>>);
  RUN_SECTION("exec: run and wait_any<0> many", test_basic<ExecPtr, create_exec, tester_wait_any<0>>);
  RUN_SECTION("exec: cancel and wait_any<0>", test_cancel<ExecPtr, create_exec, tester_wait_any<1>>);
  RUN_SECTION("exec: actor failure and wait_any<0> / sleep",
              test_failure_actor<ExecPtr, create_exec, tester_wait_any<0>, waiter_sleep6>);
  RUN_SECTION("exec: host failure and wait_any<0> / sleep",
              test_failure_host<ExecPtr, create_exec, tester_wait_any<0>, waiter_sleep6>);
  RUN_SECTION("exec: actor failure and wait_any<0> / wait",
              test_failure_actor<ExecPtr, create_exec, tester_wait_any<0>, waiter_wait>);
  RUN_SECTION("exec: host failure and wait_any<0> / wait",
              test_failure_host<ExecPtr, create_exec, tester_wait_any<0>, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait_any<1>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait_any<1> once", test_trivial<ExecPtr, create_exec, tester_wait_any<1>>);
  RUN_SECTION("exec: run and wait_any<1> many", test_basic<ExecPtr, create_exec, tester_wait_any<1>>);
  RUN_SECTION("exec: cancel and wait_any<1>", test_cancel<ExecPtr, create_exec, tester_wait_any<1>>);
  RUN_SECTION("exec: actor failure and wait_any<1> / sleep",
              test_failure_actor<ExecPtr, create_exec, tester_wait_any<1>, waiter_sleep6>);
  RUN_SECTION("exec: host failure and wait_any<1> / sleep",
              test_failure_host<ExecPtr, create_exec, tester_wait_any<1>, waiter_sleep6>);
  RUN_SECTION("exec: actor failure and wait_any<1> / wait",
              test_failure_actor<ExecPtr, create_exec, tester_wait_any<1>, waiter_wait>);
  RUN_SECTION("exec: host failure and wait_any<1> / wait",
              test_failure_host<ExecPtr, create_exec, tester_wait_any<1>, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}

// FIXME: The tests grouped here are currently failing. Once fixed, they should be put in the right section above.
//        The tests can be activated with run-time parameter '*' or, more specifically '[failing]'
TEST_CASE("Activity test/wait: tests currently failing", "[.][failing]")
{
  XBT_INFO("#####[ launch next failing test ]#####");

  // with tester_wait<0>
  // -> wait_for() should return immediately and signal a timeout (timeout == 0)
  RUN_SECTION("exec: actor failure and wait<0> / wait",
              test_failure_actor<ExecPtr, create_exec, tester_wait<0>, waiter_wait>);
  // -> wait_for() should return immediately and signal a timeout (timeout == 0)
  RUN_SECTION("exec: host failure and wait<0> / wait",
              test_failure_host<ExecPtr, create_exec, tester_wait<0>, waiter_wait>);

  // with tester_wait<1>
  // -> second call to wait_for() should wait for timeuout and not return immediately
  RUN_SECTION("exec: run and wait<1> many", test_basic<ExecPtr, create_exec, tester_wait<1>>);
  // -> second call to wait_for() should report a failure, and not a timeout
  RUN_SECTION("exec: actor failure and wait<1> / sleep",
              test_failure_actor<ExecPtr, create_exec, tester_wait<1>, waiter_sleep6>);
  // -> second call to wait_for() should report a failure, and not a timeout
  RUN_SECTION("exec: host failure and wait<1> / sleep",
              test_failure_host<ExecPtr, create_exec, tester_wait<1>, waiter_sleep6>);
  // -> actor should not be killed by TimeoutException
  RUN_SECTION("exec: actor failure and wait<1> / wait",
              test_failure_actor<ExecPtr, create_exec, tester_wait<1>, waiter_wait>);
  // -> actor should not be killed by TimeoutException
  RUN_SECTION("exec: host failure and wait<1> / wait",
              test_failure_host<ExecPtr, create_exec, tester_wait<1>, waiter_wait>);

  simgrid::s4u::this_actor::sleep_for(10);
  assert_cleanup();
}
