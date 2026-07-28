/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.ArrayList;
import java.util.List;
import org.simgrid.s4u.*;

class ChainMessage {
  public Mailbox prev;
  public Mailbox next;
  public int num_pieces;
  public ChainMessage(Mailbox prev, Mailbox next, int num_pieces)
  {
    this.prev       = prev;
    this.next       = next;
    this.num_pieces = num_pieces;
  }
}

class FilePiece {
  public FilePiece() {}
}

class Peer extends Actor {
  static final int PIECE_SIZE                    = 65536;
  static final int MESSAGE_BUILD_CHAIN_SIZE      = 40;
  static final int MESSAGE_SEND_DATA_HEADER_SIZE = 1;

  Mailbox prev = null;
  Mailbox next = null;
  Mailbox me;
  ActivitySet pending_recvs = new ActivitySet();
  ActivitySet pending_sends = new ActivitySet();

  long received_bytes = 0;
  int received_pieces = 0;
  int total_pieces    = 0;

  void joinChain() throws SimgridException
  {
    ChainMessage msg = (ChainMessage)me.get();
    prev             = msg.prev;
    next             = msg.next;
    total_pieces     = msg.num_pieces;
  }

  void forwardFile() throws SimgridException
  {
    boolean done = false;

    while (!done) {
      Comm comm = me.get_async();
      pending_recvs.push(comm);

      Activity completed_one = pending_recvs.await_any();
      if (completed_one != null) {
        comm               = (Comm)completed_one;
        FilePiece received = (FilePiece)comm.get_payload();
        if (next != null) {
          Comm send = next.put_async(received, MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
          pending_sends.push(send);
        }

        received_pieces++;
        received_bytes += PIECE_SIZE;
        if (received_pieces >= total_pieces) {
          done = true;
        }
      }
    }
  }

  public void run() throws SimgridException
  {
    me = this.get_engine().mailbox_by_name(this.get_host().get_name());

    double start_time = Engine.get_clock();
    joinChain();
    forwardFile();

    pending_sends.await_all();
    double end_time = Engine.get_clock();

    Engine.info("### %f %d bytes (Avg %f MB/s); copy finished (simulated).", end_time - start_time, received_bytes,
                received_bytes / 1024.0 / 1024.0 / (end_time - start_time));
  }
}

class Broadcaster extends Actor {
  static final int PIECE_SIZE                    = 65536;
  static final int MESSAGE_BUILD_CHAIN_SIZE      = 40;
  static final int MESSAGE_SEND_DATA_HEADER_SIZE = 1;

  Mailbox first;
  List<Mailbox> mailboxes = new ArrayList<>();
  int piece_count;

  public Broadcaster(int hostcount, int piece_count)
  {
    this.piece_count = piece_count;
    for (int i = 1; i <= hostcount; i++) {
      String name = "node-" + i + ".simgrid.org";
      mailboxes.add(this.get_engine().mailbox_by_name(name));
    }
  }

  void buildChain()
  {
    /* Build the chain if there's at least one peer */
    if (!mailboxes.isEmpty())
      first = mailboxes.get(0);

    for (int i = 0; i < mailboxes.size(); i++) {
      Mailbox prev = i > 0 ? mailboxes.get(i - 1) : null;
      Mailbox next = i < mailboxes.size() - 1 ? mailboxes.get(i + 1) : null;
      /* Send message to current peer */
      mailboxes.get(i).put(new ChainMessage(prev, next, piece_count), MESSAGE_BUILD_CHAIN_SIZE);
    }
  }

  void sendFile() throws SimgridException
  {
    ActivitySet pending_sends = new ActivitySet();
    for (int current_piece = 0; current_piece < piece_count; current_piece++) {
      Comm comm = first.put_async(new FilePiece(), MESSAGE_SEND_DATA_HEADER_SIZE + PIECE_SIZE);
      pending_sends.push(comm);
    }
    pending_sends.await_all();
  }

  public void run() throws SimgridException
  {
    buildChain();
    sendFile();
  }
}

public class app_chainsend {
  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    e.load_platform(args[0]);

    e.host_by_name("node-0.simgrid.org").add_actor("broadcaster", new Broadcaster(8, 256));

    e.host_by_name("node-1.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-2.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-3.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-4.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-5.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-6.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-7.simgrid.org").add_actor("peer", new Peer());
    e.host_by_name("node-8.simgrid.org").add_actor("peer", new Peer());

    e.run();
    Engine.info("Total simulation time: %e", Engine.get_clock());

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
