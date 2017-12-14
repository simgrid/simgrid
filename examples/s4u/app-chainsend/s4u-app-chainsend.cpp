/* Copyright (c) 2007-2010, 2012-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

#define PIECE_SIZE 65536
#define MESSAGE_BUILD_CHAIN_SIZE 40
#define MESSAGE_SEND_DATA_HEADER_SIZE 1

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_chainsend, "Messages specific for chainsend");

class ChainMessage {
public:
  simgrid::s4u::MailboxPtr prev_ = nullptr;
  simgrid::s4u::MailboxPtr next_ = nullptr;
  unsigned int num_pieces        = 0;
  explicit ChainMessage(simgrid::s4u::MailboxPtr prev, simgrid::s4u::MailboxPtr next, const unsigned int num_pieces)
      : prev_(prev), next_(next), num_pieces(num_pieces)
  {
  }
  ~ChainMessage() = default;
};

class FilePiece {
public:
  FilePiece()  = default;
  ~FilePiece() = default;
};

class Peer {
public:
  simgrid::s4u::MailboxPtr prev = nullptr;
  simgrid::s4u::MailboxPtr next = nullptr;
  simgrid::s4u::MailboxPtr me   = nullptr;
  std::vector<simgrid::s4u::CommPtr> pending_recvs;
  std::vector<simgrid::s4u::CommPtr> pending_sends;

  unsigned long long received_bytes = 0;
  unsigned int received_pieces      = 0;
  unsigned int total_pieces         = 0;

  Peer() { me = simgrid::s4u::Mailbox::byName(simgrid::s4u::Host::current()->getCname()); }
  ~Peer()     = default;

  void joinChain()
  {
    ChainMessage* msg = static_cast<ChainMessage*>(me->get());
    prev              = msg->prev_;
    next              = msg->next_;
    total_pieces      = msg->num_pieces;
    XBT_DEBUG("Peer %s got a 'BUILD_CHAIN' message (prev: %s / next: %s)", me->getCname(),
              prev ? prev->getCname() : nullptr, next ? next->getCname() : nullptr);
    delete msg;
  }

  void forwardFile()
  {
    void* received;
    bool done = false;

    while (not done) {
      simgrid::s4u::CommPtr comm = me->get_async(&received);
      pending_recvs.push_back(comm);

      int idx = simgrid::s4u::Comm::wait_any(&pending_recvs);
      if (idx != -1) {
        comm = pending_recvs.at(idx);
        XBT_DEBUG("Peer %s got a 'SEND_DATA' message", me->getCname());
        pending_recvs.erase(pending_recvs.begin() + idx);
        if (next != nullptr) {
          XBT_DEBUG("Sending (asynchronously) from %s to %s", me->getCname(), next->getCname());
          simgrid::s4u::CommPtr send = next->put_async(received, MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
          pending_sends.push_back(send);
        } else
          delete static_cast<FilePiece*>(received);

        received_pieces++;
        received_bytes += PIECE_SIZE;
        XBT_DEBUG("%u pieces received, %llu bytes received", received_pieces, received_bytes);
        if (received_pieces >= total_pieces) {
          done = true;
        }
      }
    }
  }
};

class Broadcaster {
public:
  simgrid::s4u::MailboxPtr first = nullptr;
  std::vector<simgrid::s4u::MailboxPtr> mailboxes;
  unsigned int piece_count;

  void buildChain()
  {
    auto cur                      = mailboxes.begin();
    simgrid::s4u::MailboxPtr prev = nullptr;
    simgrid::s4u::MailboxPtr last = nullptr;

    /* Build the chain if there's at least one peer */
    if (cur != mailboxes.end()) {
      /* init: prev=NULL, host=current cur, next=next cur */
      simgrid::s4u::MailboxPtr next = *cur;
      first                         = next;

      /* This iterator iterates one step ahead: cur is current iterated element, but is actually next in the chain */
      do {
        /* following steps: prev=last, host=next, next=cur */
        ++cur;
        prev                                     = last;
        simgrid::s4u::MailboxPtr current_mailbox = next;
        if (cur != mailboxes.end())
          next = *cur;
        else
          next = nullptr;

        XBT_DEBUG("Building chain--broadcaster:\"%s\" dest:\"%s\" prev:\"%s\" next:\"%s\"",
                  simgrid::s4u::Host::current()->getCname(), current_mailbox->getCname(),
                  prev ? prev->getCname() : nullptr, next ? next->getCname() : nullptr);

        /* Send message to current peer */
        current_mailbox->put(new ChainMessage(prev, next, piece_count), MESSAGE_BUILD_CHAIN_SIZE);

        last = current_mailbox;
      } while (cur != mailboxes.end());
    }
  }

  void sendFile()
  {
    std::vector<simgrid::s4u::CommPtr> pending_sends;
    for (unsigned int current_piece = 0; current_piece < piece_count; current_piece++) {
      XBT_DEBUG("Sending (send) piece %u from %s into mailbox %s", current_piece,
                simgrid::s4u::Host::current()->getCname(), first->getCname());
      simgrid::s4u::CommPtr comm = first->put_async(new FilePiece(), MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
      pending_sends.push_back(comm);
    }
    simgrid::s4u::Comm::wait_all(&pending_sends);
  }

  Broadcaster(int hostcount, unsigned int piece_count) : piece_count(piece_count)
  {
    for (int i = 1; i <= hostcount; i++) {
      std::string name = std::string("node-") + std::to_string(i) + ".acme.org";
      XBT_DEBUG("%s", name.c_str());
      mailboxes.push_back(simgrid::s4u::Mailbox::byName(name));
    }
  }

  ~Broadcaster() = default;
};

static void peer()
{
  XBT_DEBUG("peer");

  Peer* p = new Peer();

  double start_time = simgrid::s4u::Engine::getClock();
  p->joinChain();
  p->forwardFile();

  simgrid::s4u::Comm::wait_all(&p->pending_sends);
  double end_time = simgrid::s4u::Engine::getClock();

  XBT_INFO("### %f %llu bytes (Avg %f MB/s); copy finished (simulated).", end_time - start_time, p->received_bytes,
           p->received_bytes / 1024.0 / 1024.0 / (end_time - start_time));

  delete p;
}

static void broadcaster(int hostcount, unsigned int piece_count)
{
  XBT_DEBUG("broadcaster");

  Broadcaster* bc = new Broadcaster(hostcount, piece_count);
  bc->buildChain();
  bc->sendFile();

  delete bc;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("broadcaster", simgrid::s4u::Host::by_name("node-0.acme.org"), broadcaster, 8, 256);

  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-1.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-2.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-3.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-4.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-5.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-6.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-7.acme.org"), peer);
  simgrid::s4u::Actor::createActor("peer", simgrid::s4u::Host::by_name("node-8.acme.org"), peer);

  e.run();
  XBT_INFO("Total simulation time: %e", simgrid::s4u::Engine::getClock());

  return 0;
}
