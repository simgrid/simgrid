/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

constexpr unsigned PIECE_SIZE                    = 65536;
constexpr unsigned MESSAGE_BUILD_CHAIN_SIZE      = 40;
constexpr unsigned MESSAGE_SEND_DATA_HEADER_SIZE = 1;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_chainsend, "Messages specific for chainsend");
namespace sg4 = simgrid::s4u;

class ChainMessage {
public:
  sg4::Mailbox* prev_            = nullptr;
  sg4::Mailbox* next_            = nullptr;
  unsigned int num_pieces        = 0;
  explicit ChainMessage(sg4::Mailbox* prev, sg4::Mailbox* next, const unsigned int num_pieces)
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
  sg4::Mailbox* prev = nullptr;
  sg4::Mailbox* next = nullptr;
  sg4::Mailbox* me   = nullptr;
  std::vector<sg4::CommPtr> pending_recvs;
  std::vector<sg4::CommPtr> pending_sends;

  unsigned long long received_bytes = 0;
  unsigned int received_pieces      = 0;
  unsigned int total_pieces         = 0;

  Peer() { me = sg4::Mailbox::by_name(sg4::Host::current()->get_cname()); }

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
      sg4::CommPtr comm = me->get_async<FilePiece>(&received);
      pending_recvs.push_back(comm);

      ssize_t idx = sg4::Comm::wait_any(pending_recvs);
      if (idx != -1) {
        comm = pending_recvs.at(idx);
        XBT_DEBUG("Peer %s got a 'SEND_DATA' message", me->get_cname());
        pending_recvs.erase(pending_recvs.begin() + idx);
        if (next != nullptr) {
          XBT_DEBUG("Sending (asynchronously) from %s to %s", me->get_cname(), next->get_cname());
          sg4::CommPtr send = next->put_async(received, MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
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
  sg4::Mailbox* first = nullptr;
  std::vector<sg4::Mailbox*> mailboxes;
  unsigned int piece_count;

  void buildChain()
  {
    /* Build the chain if there's at least one peer */
    if (not mailboxes.empty())
      first = mailboxes.front();

    for (unsigned i = 0; i < mailboxes.size(); i++) {
      sg4::Mailbox* prev = i > 0 ? mailboxes[i - 1] : nullptr;
      sg4::Mailbox* next = i < mailboxes.size() - 1 ? mailboxes[i + 1] : nullptr;
      XBT_DEBUG("Building chain--broadcaster:\"%s\" dest:\"%s\" prev:\"%s\" next:\"%s\"",
                sg4::Host::current()->get_cname(), mailboxes[i]->get_cname(), prev ? prev->get_cname() : nullptr,
                next ? next->get_cname() : nullptr);
      /* Send message to current peer */
      mailboxes[i]->put(new ChainMessage(prev, next, piece_count), MESSAGE_BUILD_CHAIN_SIZE);
    }
  }

  void sendFile()
  {
    std::vector<sg4::CommPtr> pending_sends;
    for (unsigned int current_piece = 0; current_piece < piece_count; current_piece++) {
      XBT_DEBUG("Sending (send) piece %u from %s into mailbox %s", current_piece, sg4::Host::current()->get_cname(),
                first->get_cname());
      sg4::CommPtr comm = first->put_async(new FilePiece(), MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
      pending_sends.push_back(comm);
    }
    sg4::Comm::wait_all(pending_sends);
  }

  Broadcaster(int hostcount, unsigned int piece_count) : piece_count(piece_count)
  {
    for (int i = 1; i <= hostcount; i++) {
      std::string name = std::string("node-") + std::to_string(i) + ".simgrid.org";
      XBT_DEBUG("%s", name.c_str());
      mailboxes.push_back(sg4::Mailbox::by_name(name));
    }
  }
};

static void peer()
{
  XBT_DEBUG("peer");

  Peer p;

  double start_time = sg4::Engine::get_clock();
  p.joinChain();
  p.forwardFile();

  sg4::Comm::wait_all(p.pending_sends);
  double end_time = sg4::Engine::get_clock();

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
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("broadcaster", e.host_by_name("node-0.simgrid.org"), broadcaster, 8, 256);

  sg4::Actor::create("peer", e.host_by_name("node-1.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-2.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-3.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-4.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-5.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-6.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-7.simgrid.org"), peer);
  sg4::Actor::create("peer", e.host_by_name("node-8.simgrid.org"), peer);

  e.run();
  XBT_INFO("Total simulation time: %e", sg4::Engine::get_clock());

  return 0;
}
