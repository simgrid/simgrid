/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

std::vector<simgrid::s4u::Host*> all_hosts;

/* Helper function easing the testing of actor's ending condition */
static void assert_exit(int status, double duration)
{
  double expected_time = simgrid::s4u::Engine::get_clock() + duration;
  simgrid::s4u::this_actor::on_exit(
      [status, expected_time](int got_status, void* /*ignored*/) {
        xbt_assert(status == got_status, "Exit status mismatch. Expected %d, got %d", status, got_status);
        xbt_assert(std::fabs(expected_time - simgrid::s4u::Engine::get_clock()) < 0.001,
                   "Exit time mismatch. Expected %f", expected_time);
        XBT_VERB("Checks on exit successful");
      },
      nullptr);
}
/* Helper function in charge of running a test and doing some sanity checks afterward */
static void run_test(const char* test_name, std::function<void()> test)
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

static void test_sleep()
{
  XBT_INFO("%s: Launch a sleep(5), and let it proceed", __func__);
  bool global = false;

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5", all_hosts[1], [&global]() {
    assert_exit(0, 5.);
    simgrid::s4u::this_actor::sleep_for(5);
    global = true;
  });
  simgrid::s4u::this_actor::sleep_for(10);
  xbt_assert(global, "The forked actor did not modify the global after sleeping. Was it killed before?");
}

static void test_sleep_kill_middle()
{
  XBT_INFO("%s: Launch a sleep(5), and kill it after 2 secs", __func__);

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_killed", all_hosts[1], []() {
    assert_exit(1, 2);
    simgrid::s4u::this_actor::sleep_for(5);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::sleep_for(2);
  sleeper5->kill();
}

static void test_sleep_kill_begin()
{
  XBT_INFO("%s: Launch a sleep(5), and kill it right after start", __func__);

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_killed", all_hosts[1], []() {
    assert_exit(1, 0);
    simgrid::s4u::this_actor::sleep_for(5);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::yield();
  sleeper5->kill();
}

static void test_sleep_restart_begin()
{
  XBT_INFO("%s: Launch a sleep(5), and restart its host right after start", __func__);

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], []() {
    assert_exit(1, 0);
    simgrid::s4u::this_actor::sleep_for(5);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::yield();
  sleeper5->get_host()->turn_off();
  sleeper5->get_host()->turn_on();
  XBT_INFO("Test %s is ending", __func__);
}

static void test_sleep_restart_middle()
{
  XBT_INFO("%s: Launch a sleep(5), and restart its host after 2 secs", __func__);

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], []() {
    assert_exit(1, 2);
    simgrid::s4u::this_actor::sleep_for(5);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::sleep_for(2);
  sleeper5->get_host()->turn_off();
  sleeper5->get_host()->turn_on();
  XBT_INFO("Test %s is ending", __func__);
}
static void test_sleep_restart_end()
{
  XBT_INFO("%s: Launch a sleep(5), and restart its host right when it stops", __func__);
  bool sleeper_done = false;

  simgrid::s4u::Actor::create("sleep5_restarted", all_hosts[1], [&sleeper_done]() {
    assert_exit(0, 5);
    simgrid::s4u::this_actor::sleep_for(5);
    sleeper_done = true;
  });
  simgrid::s4u::Actor::create("killer", all_hosts[0], []() {
    simgrid::s4u::this_actor::sleep_for(5);
    XBT_INFO("Killer!");
    all_hosts[1]->turn_off();
    all_hosts[1]->turn_on();
  });
  simgrid::s4u::this_actor::sleep_for(10);
  xbt_assert(sleeper_done,
             "Restarted actor was already dead in the scheduling round during which the host_off simcall was issued");
}

static void test_comm()
{
  XBT_INFO("%s: Launch a communication", __func__);
  bool send_done = false;
  bool recv_done = false;

  simgrid::s4u::Actor::create("sender", all_hosts[1], [&send_done]() {
    assert_exit(0, 5);
    char* payload = xbt_strdup("toto");
    simgrid::s4u::Mailbox::by_name("mb")->put(payload, 5000);
    send_done = true;
  });
  simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
    assert_exit(0, 5);
    void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
    xbt_free(payload);
    recv_done = true;
  });

  simgrid::s4u::this_actor::sleep_for(10);
  xbt_assert(send_done, "Sender killed somehow. It shouldn't");
  xbt_assert(recv_done, "Receiver killed somehow. It shouldn't");
}

static void test_comm_killsend()
{
  XBT_INFO("%s: Launch a communication and kill the sender", __func__);
  bool send_done = false;
  bool recv_done = false;

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&send_done]() {
    assert_exit(1, 2);
    char* payload = xbt_strdup("toto");
    simgrid::s4u::Mailbox::by_name("mb")->put(payload, 5000);
    send_done = true;
  });
  simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
    assert_exit(0, 2);
    bool got_exception = false;
    try {
      void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
      xbt_free(payload);
    } catch (simgrid::NetworkFailureException const&) {
      got_exception = true;
    }
    xbt_assert(got_exception);
    recv_done = true;
  });

  simgrid::s4u::this_actor::sleep_for(2);
  sender->kill();

  xbt_assert(not send_done, "Sender was not killed properly");
  // xbt_assert(recv_done, "Receiver killed somehow. It shouldn't");
}

/* We need an extra actor here, so that it can sleep until the end of each test */
static void main_dispatcher()
{
  run_test("sleep", test_sleep);
  run_test("sleep killed at start", test_sleep_kill_begin);
  run_test("sleep killed in middle", test_sleep_kill_middle);
  /* We cannot kill right at the end of the action because killer actors are always rescheduled to the end of the round
   * to avoid that they exit before their victim dereferences their name */
  run_test("sleep restarted at start", test_sleep_restart_begin);
  run_test("sleep restarted at middle", test_sleep_restart_middle);
  run_test("sleep restarted at end", test_sleep_restart_end);

  run_test("comm", test_comm);
  // run_test("comm kill sender", test_comm_killsend);
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
