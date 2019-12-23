/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* We want this test to be exhaustive in term of:
 *   - communication involved (regular, asynchronous, detached, with a permanent receiver declared)
 *   - whether the send or the receive was posted first
 *
 * FIXME: Missing elements: timeouts, host/actor failures, link failures
 */

#include "simgrid/s4u.hpp"

#include <cstring>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void usage(const char* binaryName, const char* defaultSend, const char* defaultRecv)
{
  std::fprintf(stderr, "Usage: %s examples/platforms/cluster_backbone.xml <send_spec> <recv_spec>\n"
                       "where spec is a list of letters giving the kind of tests you want to see.\n"
                       "Existing sender spec:\n"
                       " r regular send\n"
                       " R regular send (after a little delay)\n"
                       " i asynchronous isend\n"
                       " I asynchronous isend (after a little delay)\n"
                       " d detached send\n"
                       " D detached send (after a little delay)\n"
                       "Existing receiver spec:\n"
                       " r regular receive\n"
                       " R regular receive (after a little delay)\n"
                       " i asynchronous irecv\n"
                       " I asynchronous irecv (after a little delay)\n"
                       " p regular receive on permanent mailbox (after a little delay)\n"
                       " P regular receive on permanent mailbox (after a little delay)\n"
                       " j irecv on permanent mailbox (after a little delay)\n"
                       " J irecv on permanent mailbox (after a little delay)\n"
                       "\n"
                       "Example 1: %s examples/platforms/cluster_backbone.xml rRiIdD rrrrrr # testing all send functions\n"
                       "Default specs: %s %s (all possible pairs)\n",
               binaryName, binaryName, defaultSend, defaultRecv);
}

static void sender(std::vector<std::string> args)
{
  XBT_INFO("Sender spec: %s", args[0].c_str());
  for (unsigned int test = 1; test <= args[0].size(); test++) {
    simgrid::s4u::this_actor::sleep_until(test * 5 - 5);
    std::string* mboxName         = new std::string("Test #" + std::to_string(test));
    simgrid::s4u::Mailbox* mbox   = simgrid::s4u::Mailbox::by_name(*mboxName);

    switch (args[0][test - 1]) {
      case 'r':
        XBT_INFO("Test %u: r (regular send)", test);
        mbox->put((void*)mboxName, 42.0);
        break;
      case 'R':
        XBT_INFO("Test %u: R (sleep + regular send)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->put((void*)mboxName, 42.0);
        break;

      case 'i':
        XBT_INFO("Test %u: i (asynchronous isend)", test);
        mbox->put_async((void*)mboxName, 42.0)->wait();
        break;
      case 'I':
        XBT_INFO("Test %u: I (sleep + isend)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->put_async((void*)mboxName, 42.0)->wait();
        break;

      case 'd':
        XBT_INFO("Test %u: d (detached send)", test);
        mbox->put_init((void*)mboxName, 42.0)->detach();
        break;
      case 'D':
        XBT_INFO("Test %u: D (sleep + detached send)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->put_init((void*)mboxName, 42.0)->detach();
        break;
      default:
        xbt_die("Unknown sender spec for test %u: '%c'", test, args[0][test - 1]);
    }
    XBT_INFO("Test %u OK", test);
  }
  simgrid::s4u::this_actor::sleep_for(0.5);
  // FIXME: we should test what happens when the process ends before the end of remote comm instead of hiding it
}

static void receiver(std::vector<std::string> args)
{
  XBT_INFO("Receiver spec: %s", args[0].c_str());
  for (unsigned int test = 1; test <= args[0].size(); test++) {
    simgrid::s4u::this_actor::sleep_until(test * 5 - 5);
    std::string mboxName          = "Test #" + std::to_string(test);
    simgrid::s4u::Mailbox* mbox   = simgrid::s4u::Mailbox::by_name(mboxName);
    void* received                = nullptr;

    switch (args[0][test - 1]) {
      case 'r':
        XBT_INFO("Test %u: r (regular receive)", test);
        received = mbox->get();
        break;
      case 'R':
        XBT_INFO("Test %u: R (sleep + regular receive)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        received = mbox->get();
        break;

      case 'i':
        XBT_INFO("Test %u: i (asynchronous irecv)", test);
        mbox->get_async(&received)->wait();
        break;
      case 'I':
        XBT_INFO("Test %u: I (sleep + asynchronous irecv)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->get_async(&received)->wait();
        break;
      case 'p':
        XBT_INFO("Test %u: p (regular receive on permanent mailbox)", test);
        mbox->set_receiver(simgrid::s4u::Actor::self());
        received = mbox->get();
        break;
      case 'P':
        XBT_INFO("Test %u: P (sleep + regular receive on permanent mailbox)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->set_receiver(simgrid::s4u::Actor::self());
        received = mbox->get();
        break;
      case 'j':
        XBT_INFO("Test %u: j (irecv on permanent mailbox)", test);
        mbox->set_receiver(simgrid::s4u::Actor::self());
        mbox->get_async(&received)->wait();
        break;
      case 'J':
        XBT_INFO("Test %u: J (sleep + irecv on permanent mailbox)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->set_receiver(simgrid::s4u::Actor::self());
        mbox->get_async(&received)->wait();
        break;
      default:
        xbt_die("Unknown receiver spec for test %u: '%c'", test, args[0][test - 1]);
    }
    const std::string* receivedStr = static_cast<std::string*>(received);
    xbt_assert(*receivedStr == mboxName);
    delete receivedStr;
    XBT_INFO("Test %u OK", test);
  }
  simgrid::s4u::this_actor::sleep_for(0.5);
}

int main(int argc, char* argv[])
{
  std::string specSend;
  std::string specRecv;
  for (char s : {'r', 'R', 'i', 'I', 'd', 'D'})
    for (char r : {'r', 'R', 'i', 'I', 'p', 'P', 'j', 'J'}) {
      specSend += s;
      specRecv += r;
    }
  std::vector<std::string> argSend{specSend.c_str()};
  std::vector<std::string> argRecv{specRecv.c_str()};

  simgrid::s4u::Engine e(&argc, argv);
  if (argc < 2) {
    usage(argv[0], specSend.c_str(), specRecv.c_str());
    return 1;
  }

  e.load_platform(argv[1]);

  if (argc >= 3) {
    argSend.clear();
    argSend.push_back(argv[2]);
  }
  if (argc >= 4) {
    argRecv.clear();
    argRecv.push_back(argv[3]);
  }
  xbt_assert(argSend.front().size() == argRecv.front().size(), "Sender and receiver spec must be of the same size");

  std::vector<simgrid::s4u::Host*> hosts = e.get_all_hosts();

  simgrid::s4u::Actor::create("sender", hosts[0], sender, argSend);
  simgrid::s4u::Actor::create("recver", hosts[1], receiver, argRecv);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
