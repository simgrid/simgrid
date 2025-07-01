/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "teshsuite/catch_simgrid.hpp"
namespace sg4 = simgrid::s4u;

//========== Creators: create an async activity

template <typename Activity> using creator_type = Activity (*)(double);

// Create a new async execution with given duration
static sg4::ExecPtr create_exec(double duration)
{
  double speed = sg4::this_actor::get_host()->get_speed();
  return sg4::this_actor::exec_async(speed * duration);
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
  const double timeout      = sg4::Engine::get_clock() + duration;
  bool ret;
  try {
    XBT_DEBUG("calling wait_for(%f)", duration);
    activity->wait_for(duration);
    XBT_DEBUG("wait_for() returned normally");
    ret = true;
  } catch (const simgrid::TimeoutException& e) {
    XBT_DEBUG("wait_for() timed out (%s)", e.what());
    INFO("wait_for() timeout should expire at expected date: " + std::to_string(timeout));
    REQUIRE(sg4::Engine::get_clock() == Approx(timeout));
    ret = false;
  } catch (const simgrid::Exception& e) {
    XBT_DEBUG("wait_for() threw an exception: %s", e.what());
    ret = true;
  }
  INFO("wait_for() should return before timeout expiration at date: " << timeout);
  REQUIRE(sg4::Engine::get_clock() <= Approx(timeout));
  return ret;
}

// Calls wait_any_for([activity], Duration / 128.0) and returns true when activity is terminated, just like test()
template <int Duration, typename Activity> bool tester_wait_any(const Activity& activity)
{
  constexpr double duration = Duration / 128.0;
  const double timeout      = sg4::Engine::get_clock() + duration;
  bool ret;
  try {
    sg4::ActivitySet set;
    set.push(activity);

    XBT_DEBUG("calling wait_any_for(%f)", duration);
    auto waited_activity = set.wait_any_for(duration);

    XBT_DEBUG("wait_any_for() returned activity %p", waited_activity.get());
    REQUIRE(waited_activity.get() == activity);
    ret = true;

  } catch (const simgrid::TimeoutException& e) {
    XBT_DEBUG("wait_any_for() timed out");
    INFO("wait_any_for() timeout should expire at expected date: " << timeout);
    REQUIRE(sg4::Engine::get_clock() == Approx(timeout));
    ret = false;
  } catch (const simgrid::Exception& e) {
    XBT_DEBUG("wait_any_for() threw an exception: %s", e.what());
    ret = true;
  }
  INFO("wait_any_for() should return before timeout expiration at date: " << timeout);
  REQUIRE(sg4::Engine::get_clock() <= Approx(timeout));
  return ret;
}

//========== Waiters: wait for the completion of an activity

template <typename Activity> using waiter_type = void (*)(const Activity&);

// Wait for 6s
template <typename Activity> void waiter_sleep6(const Activity&)
{
  sg4::this_actor::sleep_for(6.0);
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

  sg4::ActorPtr actor = all_hosts[1]->add_actor("actor", []() {
    assert_exit(true, 6.);
    Activity activity = Create(5.0);
    sg4::this_actor::sleep_for(6.0);
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  actor->join();
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test> void test_basic()
{
  XBT_INFO("Launch an activity for 5s, and test while it proceeds");

  sg4::ActorPtr actor = all_hosts[1]->add_actor("actor", []() {
    assert_exit(true, 6.);
    Activity activity = Create(5.0);
    for (int i = 0; i < 3; i++) {
      const double timestep = sg4::Engine::get_clock() + 2.0;
      INFO("activity should be still running (i = " << i << ")");
      REQUIRE(not Test(activity));
      sg4::this_actor::sleep_until(timestep);
    }
    INFO("activity should be terminated now");
    REQUIRE(Test(activity));
  });
  actor->join();
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test> void test_cancel()
{
  XBT_INFO("Launch an activity for 5s, and cancel it after 2s");

  sg4::ActorPtr actor = all_hosts[1]->add_actor("actor", []() {
    assert_exit(true, 2.);
    Activity activity = Create(5.0);
    sg4::this_actor::sleep_for(2.0);
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
  sg4::ActorPtr actor = all_hosts[1]->add_actor("actor", [&activity]() {
    assert_exit(false, 2.);
    activity = Create(5.0);
    Wait(activity);
    FAIL("should not be here!");
  });
  const double timestep = sg4::Engine::get_clock() + 2.0;
  sg4::this_actor::sleep_for(1.0);
  INFO("activity should be still running");
  REQUIRE(not Test(activity));
  sg4::this_actor::sleep_until(timestep);
  actor->kill();
  INFO("activity should be terminated now");
  REQUIRE(Test(activity));
}

template <typename Activity, creator_type<Activity> Create, tester_type<Activity> Test, waiter_type<Activity> Wait>
void test_failure_host()
{
  XBT_INFO("Launch an activity for 5s, and shutdown host 2s");

  Activity activity;
  sg4::ActorPtr actor = all_hosts[1]->add_actor("actor", [&activity]() {
    assert_exit(false, 2.);
    activity = Create(5.0);
    Wait(activity);
    FAIL("should not be here!");
  });
  const double timestep = sg4::Engine::get_clock() + 2.0;
  sg4::this_actor::sleep_for(1.0);
  INFO("activity should be still running");
  REQUIRE(not Test(activity));
  sg4::this_actor::sleep_until(timestep);
  actor->get_host()->turn_off();
  actor->get_host()->turn_on();
  INFO("activity should be terminated now");
  REQUIRE(Test(activity));
}

//==========

using sg4::ExecPtr;

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

  sg4::this_actor::sleep_for(10);
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
  RUN_SECTION("exec: actor failure and wait<0> / wait",
              test_failure_actor<ExecPtr, create_exec, tester_wait<0>, waiter_wait>);
  RUN_SECTION("exec: host failure and wait<0> / wait",
              test_failure_host<ExecPtr, create_exec, tester_wait<0>, waiter_wait>);

  sg4::this_actor::sleep_for(10);
  assert_cleanup();
}

TEST_CASE("Activity test/wait: using <tester_wait<1>>")
{
  XBT_INFO("#####[ launch next test ]#####");

  RUN_SECTION("exec: run and wait<1> once", test_trivial<ExecPtr, create_exec, tester_wait<1>>);
  RUN_SECTION("exec: run and wait<1> many", test_basic<ExecPtr, create_exec, tester_wait<1>>);
  RUN_SECTION("exec: cancel and wait<1>", test_cancel<ExecPtr, create_exec, tester_wait<1>>);
  RUN_SECTION("exec: actor failure and wait<1> / sleep",
              test_failure_actor<ExecPtr, create_exec, tester_wait<1>, waiter_sleep6>);
  RUN_SECTION("exec: host failure and wait<1> / sleep",
              test_failure_host<ExecPtr, create_exec, tester_wait<1>, waiter_sleep6>);
  RUN_SECTION("exec: actor failure and wait<1> / wait",
              test_failure_actor<ExecPtr, create_exec, tester_wait<1>, waiter_wait>);
  RUN_SECTION("exec: host failure and wait<1> / wait",
              test_failure_host<ExecPtr, create_exec, tester_wait<1>, waiter_wait>);

  sg4::this_actor::sleep_for(10);
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

  sg4::this_actor::sleep_for(10);
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

  sg4::this_actor::sleep_for(10);
  assert_cleanup();
}
