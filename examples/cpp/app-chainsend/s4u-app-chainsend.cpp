/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

constexpr unsigned PIECE_SIZE                    = 65536;
constexpr unsigned MESSAGE_BUILD_CHAIN_SIZE      = 40;
constexpr unsigned MESSAGE_SEND_DATA_HEADER_SIZE = 1;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_chainsend, "Messages specific for chainsend");

class ChainMessage {
public:
  simgrid::s4u::Mailbox* prev_   = nullptr;
  simgrid::s4u::Mailbox* next_   = nullptr;
  unsigned int num_pieces        = 0;
  explicit ChainMessage(simgrid::s4u::Mailbox* prev, simgrid::s4u::Mailbox* next, const unsigned int num_pieces)
      : prev_(prev), next_(next), num_pieces(num_pieces)
  {
  }
};

class FilePiece {
public:
  FilePiece()  = default;
};

class Peer {
public:
  simgrid::s4u::Mailbox* prev = nullptr;
  simgrid::s4u::Mailbox* next = nullptr;
  simgrid::s4u::Mailbox* me   = nullptr;
  std::vector<simgrid::s4u::CommPtr> pending_recvs;
  std::vector<simgrid::s4u::CommPtr> pending_sends;

  unsigned long long received_bytes = 0;
  unsigned int received_pieces      = 0;
  unsigned int total_pieces         = 0;

  Peer() { me = simgrid::s4u::Mailbox::by_name(simgrid::s4u::Host::current()->get_cname()); }

  void joinChain()
  {
    auto msg     = me->get_unique<ChainMessage>();
    prev         = msg->prev_;
    next         = msg->next_;
    total_pieces = msg->num_pieces;
    XBT_DEBUG("Peer %s got a 'BUILD_CHAIN' message (prev: %s / next: %s)", me->get_cname(),
              prev ? prev->get_cname() : nullptr, next ? next->get_cname() : nullptr);
  }

  void forwardFile()
  {
    FilePiece* received;
    bool done = false;

    while (not done) {
      simgrid::s4u::CommPtr comm = me->get_async<FilePiece>(&received);
      pending_recvs.push_back(comm);

      int idx = simgrid::s4u::Comm::wait_any(&pending_recvs);
      if (idx != -1) {
        comm = pending_recvs.at(idx);
        XBT_DEBUG("Peer %s got a 'SEND_DATA' message", me->get_cname());
        pending_recvs.erase(pending_recvs.begin() + idx);
        if (next != nullptr) {
          XBT_DEBUG("Sending (asynchronously) from %s to %s", me->get_cname(), next->get_cname());
          simgrid::s4u::CommPtr send = next->put_async(received, MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
          pending_sends.push_back(send);
        } else
          delete received;

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
  simgrid::s4u::Mailbox* first = nullptr;
  std::vector<simgrid::s4u::Mailbox*> mailboxes;
  unsigned int piece_count;

  void buildChain()
  {
    /* Build the chain if there's at least one peer */
    if (not mailboxes.empty())
      first = mailboxes.front();

    for (unsigned i = 0; i < mailboxes.size(); i++) {
      simgrid::s4u::Mailbox* prev = i > 0 ? mailboxes[i - 1] : nullptr;
      simgrid::s4u::Mailbox* next = i < mailboxes.size() - 1 ? mailboxes[i + 1] : nullptr;
      XBT_DEBUG("Building chain--broadcaster:\"%s\" dest:\"%s\" prev:\"%s\" next:\"%s\"",
                simgrid::s4u::Host::current()->get_cname(), mailboxes[i]->get_cname(),
                prev ? prev->get_cname() : nullptr, next ? next->get_cname() : nullptr);
      /* Send message to current peer */
      mailboxes[i]->put(new ChainMessage(prev, next, piece_count), MESSAGE_BUILD_CHAIN_SIZE);
    }
  }

  void sendFile()
  {
    std::vector<simgrid::s4u::CommPtr> pending_sends;
    for (unsigned int current_piece = 0; current_piece < piece_count; current_piece++) {
      XBT_DEBUG("Sending (send) piece %u from %s into mailbox %s", current_piece,
                simgrid::s4u::Host::current()->get_cname(), first->get_cname());
      simgrid::s4u::CommPtr comm = first->put_async(new FilePiece(), MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
      pending_sends.push_back(comm);
    }
    simgrid::s4u::Comm::wait_all(&pending_sends);
  }

  Broadcaster(int hostcount, unsigned int piece_count) : piece_count(piece_count)
  {
    for (int i = 1; i <= hostcount; i++) {
      std::string name = std::string("node-") + std::to_string(i) + ".simgrid.org";
      XBT_DEBUG("%s", name.c_str());
      mailboxes.push_back(simgrid::s4u::Mailbox::by_name(name));
    }
  }
};

static void peer()
{
  XBT_DEBUG("peer");

  Peer p;

  double start_time = simgrid::s4u::Engine::get_clock();
  p.joinChain();
  p.forwardFile();

  simgrid::s4u::Comm::wait_all(&p.pending_sends);
  double end_time = simgrid::s4u::Engine::get_clock();

  XBT_INFO("### %f %llu bytes (Avg %f MB/s); copy finished (simulated).", end_time - start_time, p.received_bytes,
           p.received_bytes / 1024.0 / 1024.0 / (end_time - start_time));
}

static void broadcaster(int hostcount, unsigned int piece_count)
{
  XBT_DEBUG("broadcaster");

  Broadcaster bc(hostcount, piece_count);
  bc.buildChain();
  bc.sendFile();
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("broadcaster", simgrid::s4u::Host::by_name("node-0.simgrid.org"), broadcaster, 8, 256);

  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-1.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-2.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-3.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-4.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-5.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-6.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-7.simgrid.org"), peer);
  simgrid::s4u::Actor::create("peer", simgrid::s4u::Host::by_name("node-8.simgrid.org"), peer);

  e.run();
  XBT_INFO("Total simulation time: %e", simgrid::s4u::Engine::get_clock());

  return 0;
}
