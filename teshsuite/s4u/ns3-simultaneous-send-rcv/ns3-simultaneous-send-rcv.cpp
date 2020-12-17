/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This test checks that ns-3 behave correctly when multiple flows finish   */
/* at the exact same time. Given the amount of simultaneous senders, it     */
/* also serves as a (small) crash test for ns-3.                            */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u tests");

const int payload            = 1000;
const int nb_message_to_send = 5;
const double sleep_time      = 5;
const int nb_sender          = 100;

int nb_messages_sent = 0;

simgrid::s4u::Mailbox* box = simgrid::s4u::Mailbox::by_name("test");

static void test_send(){
  for (int nb_message = 0; nb_message < nb_message_to_send; nb_message++) {
    nb_messages_sent++;
    XBT_VERB("start sending test #%i", nb_messages_sent);
    box->put(new int(nb_messages_sent), payload);
    XBT_VERB("done sending test #%i", nb_messages_sent);
    simgrid::s4u::this_actor::sleep_until(sleep_time * (nb_message + 1));
  }
}

static void test_receive(){
  for (int nb_message = 0; nb_message < nb_message_to_send * nb_sender; nb_message++) {
    XBT_VERB("waiting for messages");
    auto ptr = box->get_unique<int>();
    int id   = *ptr;
    XBT_VERB("received messages #%i", id);
  }
  XBT_INFO("Done receiving from %d senders, each of them sending %d messages", nb_sender, nb_message_to_send);
}


int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
 
  e.load_platform(argv[1]);

  auto hosts = e.get_all_hosts();

  simgrid::s4u::Actor::create("receiver", hosts[0], test_receive);
  for (int i = 0; i < nb_sender; i++)
    simgrid::s4u::Actor::create("sender_" + std::to_string(i), hosts[i % (e.get_host_count() - 1) + 1], test_send);

  e.run();

  return 0;
}
