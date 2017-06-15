#include <iostream>
#include <simgrid/s4u.hpp>
#include <stdlib.h>
#include <vector>

#define NUM_COMMS 1

XBT_LOG_NEW_DEFAULT_CATEGORY(mwe, "Minimum Working Example");

static void receiver()
{
  simgrid::s4u::MailboxPtr mymailbox    = simgrid::s4u::Mailbox::byName("receiver_mailbox");
  simgrid::s4u::MailboxPtr theirmailbox = simgrid::s4u::Mailbox::byName("sender_mailbox");

  std::vector<simgrid::s4u::CommPtr> pending_comms;

  XBT_INFO("Placing %d asynchronous recv requests", NUM_COMMS);
  void* data;
  for (int i = 0; i < NUM_COMMS; i++) {
    simgrid::s4u::CommPtr comm = simgrid::s4u::Comm::recv_async(mymailbox, &data);
    pending_comms.push_back(comm);
  }

  for (int i = 0; i < NUM_COMMS; i++) {
    XBT_INFO("Sleeping for 3 seconds (for the %dth time)...", i + 1);
    simgrid::s4u::this_actor::sleep_for(3.0);
    XBT_INFO("Calling wait_any() for %zu pending comms", pending_comms.size());
    std::vector<simgrid::s4u::CommPtr>::iterator ret_it =
        simgrid::s4u::Comm::wait_any(pending_comms.begin(), pending_comms.end());
    XBT_INFO("Counting the number of completed comms...");

    int count = 0;
    for (; ret_it != pending_comms.end(); count++, ret_it++)
      ;

    XBT_INFO("wait_any() replied that %d comms have completed", count);
    // xbt_assert(count == 1, "wait_any() replied that %d comms have completed, which is broken!", count);
  }
}

static void sender()
{
  simgrid::s4u::MailboxPtr mymailbox    = simgrid::s4u::Mailbox::byName("sender_mailbox");
  simgrid::s4u::MailboxPtr theirmailbox = simgrid::s4u::Mailbox::byName("receiver_mailbox");

  void* data = (void*)"data";

  for (int i = 0; i < NUM_COMMS; i++) {
    XBT_INFO("Sending a message to the receiver");
    simgrid::s4u::this_actor::send(theirmailbox, &data, 4);
    XBT_INFO("Sleeping for 1000 seconds");
    simgrid::s4u::this_actor::sleep_for(1000.0);
  }
}

int main(int argc, char** argv)
{

  simgrid::s4u::Engine* engine = new simgrid::s4u::Engine(&argc, argv);

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <xml platform file>" << std::endl;
    exit(1);
  }

  engine->loadPlatform(argv[1]);
  simgrid::s4u::Host* host = simgrid::s4u::Host::by_name("Tremblay");

  simgrid::s4u::Actor::createActor("Receiver", host, receiver);
  simgrid::s4u::Actor::createActor("Sender", host, sender);

  simgrid::s4u::Engine::instance()->run();

  return 0;
}
