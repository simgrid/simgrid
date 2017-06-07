/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

#include <cstring>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

using namespace simgrid::s4u;

static void usage(const char* binaryName, const char* defaultSend, const char* defaultRecv)
{
  std::fprintf(stderr, "Usage: %s examples/platforms/cluster.xml <send_spec> <recv_spec>\n"
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
                       "Example 1: %s examples/platforms/cluster.xml ripd rrrr # testing fancy functions\n"
                       "Default specs: %s %s (all possible pair)",
               binaryName, binaryName, defaultSend, defaultRecv);
  exit(1);
}

static void receiver(std::vector<std::string> args)
{
  XBT_INFO("Receiver spec: %s", args[0].c_str());
  for (unsigned int test = 0; test < args[0].size(); test++) {
    this_actor::sleep_until(test * 5);
    char* mboxName                = bprintf("Test #%u", test);
    simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(mboxName);

    switch (args[0][test]) {
      case 'r':
        XBT_INFO("Test %d: r (regular send)", test);
        simgrid::s4u::this_actor::send(mbox, (void*)mboxName, 42.0);
        break;
      case 'R':
        XBT_INFO("Test %d: R (sleep + regular send)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        simgrid::s4u::this_actor::send(mbox, (void*)mboxName, 42.0);
        break;

      case 'i':
        XBT_INFO("Test %d: i (asynchronous isend)", test);
        simgrid::s4u::this_actor::isend(mbox, (void*)mboxName, 42.0)->wait();
        break;
      case 'I':
        XBT_INFO("Test %d: I (sleep + isend)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        simgrid::s4u::this_actor::isend(mbox, (void*)mboxName, 42.0)->wait();
        break;

      case 'd':
        XBT_INFO("Test %d: d (detached send)", test);
        simgrid::s4u::this_actor::dsend(mbox, (void*)mboxName, 42.0);
        break;
      case 'D':
        XBT_INFO("Test %d: D (sleep + detached send)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        simgrid::s4u::this_actor::dsend(mbox, (void*)mboxName, 42.0);
        break;
      default:
        xbt_die("Unknown sender spec for test %d: '%c'", test, args[0][test]);
    }
    XBT_INFO("Test %d OK", test);
  }
}

static void sender(std::vector<std::string> args)
{
  XBT_INFO("Sender spec: %s", args[0].c_str());
  for (unsigned int test = 0; test < args[0].size(); test++) {
    this_actor::sleep_until(test * 5);
    char* mboxName                = bprintf("Test #%u", test);
    simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(mboxName);
    void* received                = nullptr;

    switch (args[0][test]) {
      case 'r':
        XBT_INFO("Test %d: r (regular receive)", test);
        received = simgrid::s4u::this_actor::recv(mbox);
        break;
      case 'R':
        XBT_INFO("Test %d: R (sleep + regular receive)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        received = simgrid::s4u::this_actor::recv(mbox);
        break;

      case 'i':
        XBT_INFO("Test %d: i (asynchronous irecv)", test);
        simgrid::s4u::this_actor::irecv(mbox, &received)->wait();
        break;
      case 'I':
        XBT_INFO("Test %d: I (sleep + asynchronous irecv)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        simgrid::s4u::this_actor::irecv(mbox, &received)->wait();
        break;
      case 'p':
        XBT_INFO("Test %d: p (regular receive on permanent mailbox)", test);
        mbox->setReceiver(Actor::self());
        received = simgrid::s4u::this_actor::recv(mbox);
        break;
      case 'P':
        XBT_INFO("Test %d: P (sleep + regular receive on permanent mailbox)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->setReceiver(Actor::self());
        received = simgrid::s4u::this_actor::recv(mbox);
        break;
      case 'j':
        XBT_INFO("Test %d: j (irecv on permanent mailbox)", test);
        mbox->setReceiver(Actor::self());
        simgrid::s4u::this_actor::irecv(mbox, &received)->wait();
        break;
      case 'J':
        XBT_INFO("Test %d: J (sleep + irecv on permanent mailbox)", test);
        simgrid::s4u::this_actor::sleep_for(0.5);
        mbox->setReceiver(Actor::self());
        simgrid::s4u::this_actor::irecv(mbox, &received)->wait();
        break;
      default:
        xbt_die("Unknown receiver spec for test %d: '%c'", test, args[0][test]);
    }

    xbt_assert(strcmp(static_cast<char*>(received), mboxName) == 0);
    xbt_free(received);
    xbt_free(mboxName);
    XBT_INFO("Test %d OK", test);
  }
}

int main(int argc, char* argv[])
{
  std::string specSend;
  std::string specRecv;
  for (char s : {'r', 'R', 'i', 'I', 'p', 'P', 'j', 'J'})
    for (char r : {'r', 'R', 'i', 'I', 'd', 'D'}) {
      specSend += s;
      specRecv += r;
    }
  std::vector<std::string> argSend{specSend.c_str()};
  std::vector<std::string> argRecv{specRecv.c_str()};

  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);
  if (argc < 2)
    usage(argv[0], specSend.c_str(), specRecv.c_str());

  e->loadPlatform(argv[1]);

  if (argc >= 3) {
    argSend.clear();
    argSend.push_back(argv[2]);
  }
  if (argc >= 4) {
    argRecv.clear();
    argRecv.push_back(argv[3]);
  }
  xbt_assert(argSend.front().size() == argRecv.front().size(), "Sender and receiver spec must be of the same size");

  simgrid::s4u::Host** hosts = sg_host_list();
  simgrid::s4u::Actor::createActor("sender", hosts[0], sender, argSend);
  simgrid::s4u::Actor::createActor("recver", hosts[1], receiver, argRecv);
  xbt_free(hosts);

  e->run();
  XBT_INFO("Simulation time %g", e->getClock());

  return 0;
}
