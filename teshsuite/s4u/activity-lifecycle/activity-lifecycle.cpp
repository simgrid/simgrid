/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

#include <cmath>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

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

static void test_sleep()
{
  XBT_INFO("%s: Launch a sleep(5), and let it proceed", __func__);
  bool global = false;

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5", all_hosts[1], [&global]() {
    assert_exit(false, 5.);
    simgrid::s4u::this_actor::sleep_for(5);
    global = true;
  });
  simgrid::s4u::this_actor::sleep_for(9);
  xbt_assert(global, "The forked actor did not modify the global after sleeping. Was it killed before?");
}

static void test_sleep_kill_middle()
{
  XBT_INFO("%s: Launch a sleep(5), and kill it after 2 secs", __func__);

  simgrid::s4u::ActorPtr sleeper5 = simgrid::s4u::Actor::create("sleep5_killed", all_hosts[1], []() {
    assert_exit(true, 2);
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
    assert_exit(true, 0);
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
    assert_exit(true, 0);
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
    assert_exit(true, 2);
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
    assert_exit(true, 5);
    simgrid::s4u::this_actor::sleep_for(5);
    all_hosts[1]->turn_off(); // kill the host right at the end of this sleep and of this actor
    sleeper_done = true;
  });
  simgrid::s4u::this_actor::sleep_for(9);
  xbt_assert(sleeper_done, "Not sure of how the actor survived the shutdown of its host.");
  all_hosts[1]->turn_on();
}
static void test_exec()
{
  XBT_INFO("%s: Launch a execute(5s), and let it proceed", __func__);
  bool global = false;

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5", all_hosts[1], [&global]() {
    assert_exit(false, 5.);
    simgrid::s4u::this_actor::execute(500000000);
    global = true;
  });
  simgrid::s4u::this_actor::sleep_for(9);
  xbt_assert(global, "The forked actor did not modify the global after executing. Was it killed before?");
}

static void test_exec_kill_middle()
{
  XBT_INFO("%s: Launch a execute(5s), and kill it after 2 secs", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_killed", all_hosts[1], []() {
    assert_exit(true, 2);
    simgrid::s4u::this_actor::execute(500000000);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::sleep_for(2);
  exec5->kill();
}

static void test_exec_kill_begin()
{
  XBT_INFO("%s: Launch a execute(5s), and kill it right after start", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_killed", all_hosts[1], []() {
    assert_exit(true, 0);
    simgrid::s4u::this_actor::execute(500000000);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::yield();
  exec5->kill();
}

static void test_exec_restart_begin()
{
  XBT_INFO("%s: Launch a execute(5s), and restart its host right after start", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_restarted", all_hosts[1], []() {
    assert_exit(1, 0);
    simgrid::s4u::this_actor::execute(500000000);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::yield();
  exec5->get_host()->turn_off();
  exec5->get_host()->turn_on();
  XBT_INFO("Test %s is ending", __func__);
}

static void test_exec_restart_middle()
{
  XBT_INFO("%s: Launch a execute(5s), and restart its host after 2 secs", __func__);

  simgrid::s4u::ActorPtr exec5 = simgrid::s4u::Actor::create("exec5_restarted", all_hosts[1], []() {
    assert_exit(true, 2);
    simgrid::s4u::this_actor::execute(500000000);
    xbt_die("I should be dead now");
  });

  simgrid::s4u::this_actor::sleep_for(2);
  exec5->get_host()->turn_off();
  exec5->get_host()->turn_on();
  XBT_INFO("Test %s is ending", __func__);
}
static void test_exec_restart_end()
{
  XBT_INFO("%s: Launch a execute(5s), and restart its host right when it stops", __func__);
  bool execution_done = false;

  simgrid::s4u::Actor::create("exec5_restarted", all_hosts[1], [&execution_done]() {
    assert_exit(false, 5);
    simgrid::s4u::this_actor::execute(500000000);
    execution_done = true;
  });
  simgrid::s4u::Actor::create("killer", all_hosts[0], []() {
    simgrid::s4u::this_actor::sleep_for(5);
    XBT_INFO("Killer!");
    all_hosts[1]->turn_off();
    all_hosts[1]->turn_on();
  });
  simgrid::s4u::this_actor::sleep_for(9);
  xbt_assert(execution_done,
             "Restarted actor was already dead in the scheduling round during which the host_off simcall was issued");
}

static void test_comm()
{
  XBT_INFO("%s: Launch a communication", __func__);
  bool send_done = false;
  bool recv_done = false;

  simgrid::s4u::Actor::create("sender", all_hosts[1], [&send_done]() {
    assert_exit(false, 5);
    char* payload = xbt_strdup("toto");
    simgrid::s4u::Mailbox::by_name("mb")->put(payload, 5000);
    send_done = true;
  });
  simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
    assert_exit(false, 5);
    void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
    xbt_free(payload);
    recv_done = true;
  });

  simgrid::s4u::this_actor::sleep_for(9);
  xbt_assert(send_done, "Sender killed somehow. It shouldn't");
  xbt_assert(recv_done, "Receiver killed somehow. It shouldn't");
}

static void test_comm_dsend_and_quit_put_before_get()
{
  XBT_INFO("%s: Launch a detached communication and end right after", __func__);
  bool dsend_done = false;
  bool recv_done  = false;

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&dsend_done]() {
    assert_exit(false, 0);
    char* payload = xbt_strdup("toto");
    simgrid::s4u::Mailbox::by_name("mb")->put_init(payload, 1000)->detach();
    dsend_done = true;
  });

  simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
    assert_exit(false, 3);
    simgrid::s4u::this_actor::sleep_for(2);
    void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
    xbt_free(payload);
    recv_done = true;
  });

  // Sleep long enough to let the test ends by itself. 3 + surf_precision should be enough.
  simgrid::s4u::this_actor::sleep_for(4);
  xbt_assert(dsend_done, "Sender killed somehow. It shouldn't");
  xbt_assert(recv_done, "Receiver killed somehow. It shouldn't");
}

static void test_comm_dsend_and_quit_get_before_put()
{
  XBT_INFO("%s: Launch a detached communication and end right after", __func__);
  bool dsend_done = false;
  bool recv_done  = false;

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&dsend_done]() {
    assert_exit(false, 2);
    char* payload = xbt_strdup("toto");
    simgrid::s4u::this_actor::sleep_for(2);
    simgrid::s4u::Mailbox::by_name("mb")->put_init(payload, 1000)->detach();
    dsend_done = true;
  });

  simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
    assert_exit(false, 3);
    void* payload = simgrid::s4u::Mailbox::by_name("mb")->get();
    xbt_free(payload);
    recv_done = true;
  });

  // Sleep long enough to let the test ends by itself. 3 + surf_precision should be enough.
  simgrid::s4u::this_actor::sleep_for(4);
  xbt_assert(dsend_done, "Sender killed somehow. It shouldn't");
  xbt_assert(recv_done, "Receiver killed somehow. It shouldn't");
}


static void test_comm_killsend()
{
  XBT_INFO("%s: Launch a communication and kill the sender", __func__);
  bool send_done = false;
  bool recv_done = false;

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[1], [&send_done]() {
    assert_exit(true, 2);
    // Encapsulate the payload in a std::unique_ptr so that it is correctly free'd when the sender is killed during its
    // communication (thanks to RAII).  The pointer is then released when the communication is over.
    std::unique_ptr<char, decltype(&xbt_free_f)> payload(xbt_strdup("toto"), &xbt_free_f);
    simgrid::s4u::Mailbox::by_name("mb")->put(payload.get(), 5000);
    payload.release();
    send_done = true;
  });
  simgrid::s4u::Actor::create("receiver", all_hosts[2], [&recv_done]() {
    assert_exit(false, 2);
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
  // let the test ends by itself. waiting for surf_precision should be enough.
  simgrid::s4u::this_actor::sleep_for(0.00001);

  xbt_assert(not send_done, "Sender was not killed properly");
  xbt_assert(recv_done, "Receiver killed somehow. It shouldn't");
}

static void test_host_off_while_receive()
{
  XBT_INFO("%s: Launch an actor that waits on a recv, kill its host", __func__);
  bool in_on_exit = false;
  bool returned_from_main = false;
  bool in_catch_before_on_exit = false;
  bool in_catch_after_on_exit = false;
  bool send_done               = false;

  simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create(
    "receiver", all_hosts[1], 
    [&in_on_exit, &returned_from_main, &in_catch_before_on_exit, &in_catch_after_on_exit]() {
       assert_exit(true, 1);
       try {
         simgrid::s4u::Mailbox::by_name("mb")->get();
       } catch (simgrid::NetworkFailureException const&) {
         // Shouldn't get in here
         in_catch_before_on_exit = not in_on_exit;
         in_catch_after_on_exit = in_on_exit;
       }
       returned_from_main = true;
     });

  receiver->on_exit([&in_on_exit](bool) { in_on_exit = true; });

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], [&send_done]() {
    assert_exit(false, 1);
    bool got_exception = false;
    try {
      int data = 42;
      simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000);
    } catch (simgrid::NetworkFailureException const&) {
      got_exception = true;
    }
    xbt_assert(got_exception);
    send_done = true;
  });

  simgrid::s4u::this_actor::sleep_for(1);
  receiver->get_host()->turn_off();
  
  // Note: If we don't sleep here, we don't "see" the bug
  simgrid::s4u::this_actor::sleep_for(1);

  xbt_assert(in_on_exit, 
    "Receiver's on_exit function was never called");
  xbt_assert(not in_catch_before_on_exit, 
    "Receiver mistakenly went to catch clause (before the on_exit function was called)");
  xbt_assert(not in_catch_after_on_exit, 
    "Receiver mistakenly went to catch clause (after the on_exit function was called)");
  xbt_assert(not returned_from_main, 
    "Receiver returned from main normally even though its host was killed");
  xbt_assert(send_done, "Sender killed somehow. It shouldn't");
  receiver->get_host()->turn_on();
}

static void test_link_off_helper(double delay)
{
  const double start = simgrid::s4u::Engine::get_clock();

  simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create("receiver", all_hosts[1], [&start]() {
    assert_exit(false, 9);
    double milestone[5] = {0.5, 3.5, 4.5, 7.5, 9.0};
    for (int i = 0; i < 5; i++)
      milestone[i] += start;
    for (int i = 0; i < 4; i++) {
      simgrid::s4u::this_actor::sleep_until(milestone[i]);
      try {
        XBT_VERB("get(%c)", 'A' + i);
        simgrid::s4u::Mailbox::by_name("mb")->get();
        return;
      } catch (simgrid::NetworkFailureException const&) {
        XBT_VERB("got expected NetworkFailureException");
      }
    }
    simgrid::s4u::this_actor::sleep_until(milestone[4]);
  });

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], [&start]() {
    assert_exit(false, 9);
    int data            = 42;
    double milestone[5] = {1.5, 2.5, 5.5, 6.5, 9.0};
    for (int i = 0; i < 5; i++)
      milestone[i] += start;
    for (int i = 0; i < 2; i++) {
      simgrid::s4u::this_actor::sleep_until(milestone[i]);
      XBT_VERB("dsend(%c)", 'A' + i);
      simgrid::s4u::Mailbox::by_name("mb")->put_init(&data, 100000)->detach();
    }
    for (int i = 2; i < 4; i++) {
      simgrid::s4u::this_actor::sleep_until(milestone[i]);
      try {
        XBT_VERB("put(%c)", 'A' + i);
        simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000);
        return;
      } catch (simgrid::NetworkFailureException const&) {
        XBT_VERB("got expected NetworkFailureException");
      }
    }
    simgrid::s4u::this_actor::sleep_until(milestone[4]);
  });

  for (int i = 0; i < 4; i++) {
    XBT_VERB("##### %d / 4 #####", i + 1);
    simgrid::s4u::this_actor::sleep_for(delay);
    XBT_VERB("link off");
    simgrid::s4u::Link::by_name("link1")->turn_off();
    simgrid::s4u::this_actor::sleep_for(2.0 - delay);
    XBT_VERB("link on");
    simgrid::s4u::Link::by_name("link1")->turn_on();
  }
  simgrid::s4u::this_actor::sleep_for(1.5);
}

static void test_link_off_before_send_recv()
{
  XBT_INFO("%s: try to communicate with communicating link turned off before start", __func__);
  test_link_off_helper(0.0);
}
static void test_link_off_between_send_recv()
{
  XBT_INFO("%s: try to communicate with communicating link turned off between send and receive", __func__);
  test_link_off_helper(1.0);
}
static void test_link_off_during_transfer()
{
  XBT_INFO("%s: try to communicate with communicating link turned off during transfer", __func__);
  test_link_off_helper(2.0);
}

static void test_link_off_during_wait_any()
{
  simgrid::s4u::ActorPtr receiver = simgrid::s4u::Actor::create("receiver", all_hosts[1], []() {
    assert_exit(false, 2);
    bool receiver_got_network_failure_execution = false;
    bool receiver_got_base_execution = false;
    int *data;
    std::vector<simgrid::s4u::CommPtr> pending_comms;
    simgrid::s4u::CommPtr comm = simgrid::s4u::Mailbox::by_name("mb")->get_async((void**)&data);
    pending_comms.push_back(comm);
    try {
      simgrid::s4u::Comm::wait_any(&pending_comms);
    } catch (simgrid::NetworkFailureException const&) {
      XBT_VERB("got expected NetworkFailureException");
      receiver_got_network_failure_execution = true;
    } catch (simgrid::Exception const&) {
      XBT_VERB("got unexpected base Exception");
      receiver_got_base_execution = true;
    }
    xbt_assert(receiver_got_network_failure_execution, "The receiver should have gotten a NetworkFailureException");
    xbt_assert(not receiver_got_base_execution, "The receiver should not have gotten a base Exception");
  });

  simgrid::s4u::ActorPtr sender = simgrid::s4u::Actor::create("sender", all_hosts[2], []() {
    assert_exit(false, 2);
    int data = 42;
    bool sender_got_network_failure_execution = false;
    bool sender_got_base_execution = false;
    try {
      simgrid::s4u::Mailbox::by_name("mb")->put(&data, 100000);
    } catch (simgrid::NetworkFailureException const&) {
      XBT_VERB("got expected NetworkFailureException");
      sender_got_network_failure_execution = true;
    } catch (simgrid::Exception const&) {
      XBT_VERB("got unexpected base Exception");
      sender_got_base_execution = true;
    }
    xbt_assert(sender_got_network_failure_execution, "The sender should have gotten a NetworkFailureException");
    xbt_assert(not sender_got_base_execution, "The sender should not have gotten a base Exception");
  });

  simgrid::s4u::this_actor::sleep_for(2.0);
  XBT_VERB("link off");
  simgrid::s4u::Link::by_name("link1")->turn_off();
  simgrid::s4u::this_actor::sleep_for(2.0);
  XBT_VERB("link on");
  simgrid::s4u::Link::by_name("link1")->turn_on();
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
  run_test("sleep restarted in middle", test_sleep_restart_middle);
  // run_test("sleep restarted at end", test_sleep_restart_end);

  run_test("exec", test_exec);
  run_test("exec killed at start", test_exec_kill_begin);
  run_test("exec killed in middle", test_exec_kill_middle);
  run_test("exec restarted at start", test_exec_restart_begin);
  run_test("exec restarted in middle", test_exec_restart_middle);
  run_test("exec restarted at end", test_exec_restart_end);

  run_test("comm", test_comm);
  run_test("comm dsend and quit (put before get)", test_comm_dsend_and_quit_put_before_get);
  run_test("comm dsend and quit (get before put)", test_comm_dsend_and_quit_get_before_put);
  run_test("comm kill sender", test_comm_killsend);

  run_test("comm recv and kill", test_host_off_while_receive);
  run_test("comm turn link off before send/recv", test_link_off_before_send_recv);
  run_test("comm turn link off between send/recv", test_link_off_between_send_recv);
  run_test("comm turn link off during transfer", test_link_off_during_transfer);
  run_test("comm turn link off during wait_any", test_link_off_during_wait_any);
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
