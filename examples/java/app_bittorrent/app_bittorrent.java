/* Copyright (c) 2012-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import org.simgrid.s4u.*;

/* ------------------------------------------------------------------------ */
/* Definitions shared by the tracker and the peers.                         */
/* ------------------------------------------------------------------------ */

/** Types of messages exchanged between two peers. */
enum MessageType { HANDSHAKE, CHOKE, UNCHOKE, INTERESTED, NOTINTERESTED, HAVE, BITFIELD, REQUEST, PIECE, CANCEL }

class Message {
  public MessageType type;
  public int peer_id;
  public Mailbox return_mailbox;
  public int bitfield     = 0;
  public int piece        = 0;
  public int block_index  = 0;
  public int block_length = 0;

  public Message(MessageType type, int peer_id, Mailbox return_mailbox)
  {
    this.type           = type;
    this.peer_id        = peer_id;
    this.return_mailbox = return_mailbox;
  }
  public Message(MessageType type, int peer_id, int bitfield, Mailbox return_mailbox)
  {
    this.type           = type;
    this.peer_id        = peer_id;
    this.return_mailbox = return_mailbox;
    this.bitfield       = bitfield;
  }
  public Message(MessageType type, int peer_id, Mailbox return_mailbox, int piece, int block_index, int block_length)
  {
    this.type           = type;
    this.peer_id        = peer_id;
    this.return_mailbox = return_mailbox;
    this.piece          = piece;
    this.block_index    = block_index;
    this.block_length   = block_length;
  }
  public Message(MessageType type, int peer_id, Mailbox return_mailbox, int piece)
  {
    this.type           = type;
    this.peer_id        = peer_id;
    this.return_mailbox = return_mailbox;
    this.piece          = piece;
  }
}

/* ------------------------------------------------------------------------ */
/* The tracker                                                              */
/* ------------------------------------------------------------------------ */

class TrackerQuery {
  private int peer_id;
  private Mailbox return_mailbox;

  public TrackerQuery(int peer_id, Mailbox return_mailbox)
  {
    this.peer_id        = peer_id;
    this.return_mailbox = return_mailbox;
  }
  public int getPeerId() { return peer_id; }
  public Mailbox getReturnMailbox() { return return_mailbox; }
}

class TrackerAnswer {
  private Set<Integer> peers = new LinkedHashSet<>();

  public TrackerAnswer(int interval) { /* interval unused for now, just like in the C++ version */ }
  public void addPeer(int peer) { peers.add(peer); }
  public Set<Integer> getPeers() { return peers; }
}

class Tracker extends Actor {
  double deadline;
  Random random = new Random(42);
  Mailbox mailbox;
  Set<Integer> known_peers = new LinkedHashSet<>();

  public Tracker(String[] args)
  {
    /* Checking arguments */
    if (args.length != 1)
      Engine.die("Wrong number of arguments for the tracker.");
    /* Retrieving end time */
    try {
      deadline = Double.parseDouble(args[0]);
    } catch (NumberFormatException e) {
      throw new IllegalArgumentException("Invalid deadline:" + args[0]);
    }
    if (deadline <= 0)
      Engine.die("Wrong deadline supplied");

    mailbox = this.get_engine().mailbox_by_name(app_bittorrent.TRACKER_MAILBOX);

    Engine.info("Tracker launched.");
  }

  public void run() throws SimgridException
  {
    Comm comm          = null;
    TrackerQuery query = null;
    while (Engine.get_clock() < deadline) {
      if (comm == null)
        comm = mailbox.get_async();
      if (comm.test()) {
        /* Retrieve the data sent by the peer. */
        query = (TrackerQuery)comm.get_payload();

        /* Add the peer to our peer list, if not already known. */
        known_peers.add(query.getPeerId());

        /* Sending back peers to the requesting peer */
        TrackerAnswer answer           = new TrackerAnswer(app_bittorrent.TRACKER_QUERY_INTERVAL);
        List<Integer> known_peers_list = new ArrayList<>(known_peers);
        int nb_known_peers             = known_peers_list.size();
        int max_tries                  = Math.min(app_bittorrent.MAXIMUM_PEERS, nb_known_peers);
        int tried                      = 0;
        while (tried < max_tries) {
          int next_peer;
          do {
            next_peer = known_peers_list.get(random.nextInt(nb_known_peers));
          } while (answer.getPeers().contains(next_peer));
          answer.addPeer(next_peer);
          tried++;
        }
        query.getReturnMailbox().put_init(answer, app_bittorrent.TRACKER_COMM_SIZE).detach();

        query = null;
        comm  = null;
      } else {
        this.sleep_for(1);
      }
    }
    Engine.info("Tracker is leaving");
  }
}

/* ------------------------------------------------------------------------ */
/* The peer.                                                                */
/* ------------------------------------------------------------------------ */

class Connection {
  public int id; // Peer id
  public Mailbox mailbox;
  public int bitfield            = 0; // Fields
  public double peer_speed       = 0;
  public double last_unchoke     = 0;
  public int current_piece       = -1;
  public boolean am_interested   = false; // Indicates if we are interested in something the peer has
  public boolean interested      = false; // Indicates if the peer is interested in one of our pieces
  public boolean choked_upload   = true;  // Indicates if the peer is choked for the current peer
  public boolean choked_download = true;  // Indicates if the peer has choked the current peer

  public Connection(int id, Engine e)
  {
    this.id      = id;
    this.mailbox = e.mailbox_by_name(String.valueOf(id));
  }
  public void addSpeedValue(double speed) { peer_speed = peer_speed * 0.6 + speed * 0.4; }
  public boolean hasPiece(int piece) { return (bitfield & (1 << piece)) != 0; }
}

class Peer extends Actor {
  /*
   * User parameters for transferred file data. For the test, the default values are :
   * File size: 10 pieces * 5 blocks/piece * 16384 bytes/block = 819200 bytes
   */
  static final int FILE_PIECES   = 10;
  static final int PIECES_BLOCKS = 5;
  static final int BLOCK_SIZE    = 16384;

  /** Number of blocks asked by each request */
  static final int BLOCKS_REQUESTED = 2;

  static final double SLEEP_DURATION = 1.0;

  static int bits_to_bytes(int x) { return ((x / 8 + x % 8) != 0) ? 1 : 0; }

  /**
   * Message sizes
   * Sizes based on report by A. Legout et al, Understanding BitTorrent: An Experimental Perspective
   * http://hal.inria.fr/inria-00000156/en
   */
  static long message_size(MessageType type)
  {
    switch (type) {
      case HANDSHAKE:
        return 68;
      case CHOKE:
        return 5;
      case UNCHOKE:
        return 5;
      case INTERESTED:
        return 5;
      case NOTINTERESTED:
        return 5;
      case HAVE:
        return 9;
      case BITFIELD:
        return 5;
      case REQUEST:
        return 17;
      case PIECE:
        return 13;
      case CANCEL:
        return 17;
      default:
        throw new IllegalStateException("Impossible message type");
    }
  }

  static String message_name(MessageType type) { return type.toString(); }

  int id;
  double deadline;
  Random random;
  Mailbox mailbox;
  Map<Integer, Connection> connected_peers = new LinkedHashMap<>();
  Set<Connection> active_peers             = new LinkedHashSet<>(); // active peers list

  int bitfield_             = 0;                      // list of pieces the peer has.
  long bitfield_blocks      = 0;                      // list of blocks the peer has.
  short[] pieces_count      = new short[FILE_PIECES]; // number of peers that have each piece.
  int current_pieces        = 0;                      // current pieces the peer is downloading
  double begin_receive_time = 0; // time when the receiving communication has begun, useful for calculating host speed.
  int round_                = 0; // current round for the chocking algorithm.

  Comm comm_received = null; // current comm
  Message message    = null; // current message being received

  public Peer(String[] args)
  {
    /* Check arguments */
    if (args.length != 2 && args.length != 3)
      Engine.die("Wrong number of arguments");
    try {
      id      = Integer.parseInt(args[0]);
      mailbox = this.get_engine().mailbox_by_name(String.valueOf(id));
    } catch (NumberFormatException e) {
      throw new IllegalArgumentException("Invalid ID:" + args[0]);
    }
    random = new Random(id);

    try {
      deadline = Double.parseDouble(args[1]);
    } catch (NumberFormatException e) {
      throw new IllegalArgumentException("Invalid deadline:" + args[1]);
    }
    if (deadline <= 0)
      Engine.die("Wrong deadline supplied");

    if (args.length == 3 && args[2].equals("1")) {
      bitfield_       = (1 << FILE_PIECES) - 1;
      bitfield_blocks = (1L << (FILE_PIECES * PIECES_BLOCKS)) - 1L;
    }

    Engine.info("Hi, I'm joining the network with id %d", id);
  }

  /** Peer main function */
  public void run() throws SimgridException
  {
    /* Getting peer data from the tracker. */
    if (getPeersFromTracker()) {
      Engine.debug("Got %d peers from the tracker. Current status is: %s", connected_peers.size(), getStatus());
      begin_receive_time = Engine.get_clock();
      mailbox.set_receiver(this);
      if (hasFinished()) {
        sendHandshakeToAllPeers();
      } else {
        leech();
      }
      seed();
    } else {
      Engine.info("Couldn't contact the tracker.");
    }

    Engine.info("Here is my current status: %s", getStatus());
  }

  boolean getPeersFromTracker() throws SimgridException
  {
    Mailbox tracker_mailbox = this.get_engine().mailbox_by_name(app_bittorrent.TRACKER_MAILBOX);
    /* Build the message to send to the tracker */
    TrackerQuery peer_request = new TrackerQuery(id, mailbox);
    try {
      Engine.debug("Sending a peer request to the tracker.");
      Comm comm = tracker_mailbox.put_async(peer_request, app_bittorrent.TRACKER_COMM_SIZE);
      comm.await_for(app_bittorrent.GET_PEERS_TIMEOUT);
    } catch (TimeoutException e) {
      Engine.debug("Timeout expired when requesting peers to tracker");
      return false;
    }

    try {
      Comm comm = mailbox.get_async();
      comm.await_for(app_bittorrent.GET_PEERS_TIMEOUT);
      TrackerAnswer answer = (TrackerAnswer)comm.get_payload();
      /* Add the peers the tracker gave us to our peer list. */
      for (int peer_id : answer.getPeers())
        if (id != peer_id)
          connected_peers.putIfAbsent(peer_id, new Connection(peer_id, this.get_engine()));
    } catch (TimeoutException e) {
      Engine.debug("Timeout expired when requesting peers to tracker");
      return false;
    }
    return true;
  }

  void sendHandshakeToAllPeers()
  {
    for (Connection remote_peer : connected_peers.values()) {
      Message handshake = new Message(MessageType.HANDSHAKE, id, mailbox);
      remote_peer.mailbox.put_init(handshake, message_size(MessageType.HANDSHAKE)).detach();
      Engine.debug("Sending a HANDSHAKE to %d", remote_peer.id);
    }
  }

  void sendMessage(Mailbox mailbox, MessageType type, long size)
  {
    Engine.debug("Sending %s to %s", message_name(type), mailbox.get_name());
    mailbox.put_init(new Message(type, id, bitfield_, this.mailbox), size).detach();
  }

  void sendBitfield(Mailbox mailbox)
  {
    Engine.debug("Sending a BITFIELD to %s", mailbox.get_name());
    mailbox
        .put_init(new Message(MessageType.BITFIELD, id, bitfield_, this.mailbox),
                  message_size(MessageType.BITFIELD) + bits_to_bytes(FILE_PIECES))
        .detach();
  }

  void sendPiece(Mailbox mailbox, int piece, int block_index, int block_length)
  {
    if (hasNotPiece(piece))
      Engine.die("Tried to send a unavailable piece.");
    Engine.debug("Sending the PIECE %d (%d,%d) to %s", piece, block_index, block_length, mailbox.get_name());
    mailbox.put_init(new Message(MessageType.PIECE, id, this.mailbox, piece, block_index, block_length), BLOCK_SIZE)
        .detach();
  }

  void sendHaveToAllPeers(int piece)
  {
    Engine.debug("Sending HAVE message to all my peers");
    for (Connection remote_peer : connected_peers.values()) {
      remote_peer.mailbox.put_init(new Message(MessageType.HAVE, id, mailbox, piece), message_size(MessageType.HAVE))
          .detach();
    }
  }

  void sendRequestTo(Connection remote_peer, int piece)
  {
    remote_peer.current_piece = piece;
    int block_index           = getFirstMissingBlockFrom(piece);
    if (block_index != -1) {
      int block_length = Math.min(BLOCKS_REQUESTED, PIECES_BLOCKS - block_index);
      Engine.debug("Sending a REQUEST to %s for piece %d (%d,%d)", remote_peer.mailbox.get_name(), piece, block_index,
                   block_length);
      remote_peer.mailbox
          .put_init(new Message(MessageType.REQUEST, id, mailbox, piece, block_index, block_length),
                    message_size(MessageType.REQUEST))
          .detach();
    }
  }

  String getStatus()
  {
    StringBuilder res = new StringBuilder();
    for (int i = 0; i < FILE_PIECES; i++)
      res.append((bitfield_ & (1 << i)) != 0 ? '1' : '0');
    return res.toString();
  }

  boolean hasFinished() { return bitfield_ == (1 << FILE_PIECES) - 1; }

  /** Indicates if the remote peer has a piece not stored by the local peer */
  boolean isInterestedBy(Connection remote_peer)
  {
    return (remote_peer.bitfield & (bitfield_ ^ ((1 << FILE_PIECES) - 1))) != 0;
  }

  boolean isInterestedByFree(Connection remote_peer)
  {
    for (int i = 0; i < FILE_PIECES; i++)
      if (hasNotPiece(i) && remote_peer.hasPiece(i) && isNotDownloadingPiece(i))
        return true;
    return false;
  }

  void updatePiecesCountFromBitfield(int bitfield)
  {
    for (int i = 0; i < FILE_PIECES; i++)
      if ((bitfield & (1 << i)) != 0)
        pieces_count[i]++;
  }

  boolean hasNotPiece(int piece) { return (bitfield_ & (1 << piece)) == 0; }

  boolean remotePeerHasMissingPiece(Connection remote_peer, int piece)
  {
    return hasNotPiece(piece) && remote_peer.hasPiece(piece);
  }

  boolean isNotDownloadingPiece(int piece) { return (current_pieces & (1 << piece)) == 0; }

  int countPieces(int bitfield)
  {
    int count = 0;
    int n     = bitfield;
    while (n != 0) {
      count += n & 1;
      n >>>= 1;
    }
    return count;
  }

  int nbInterestedPeers()
  {
    int count = 0;
    for (Connection remote_peer : connected_peers.values())
      if (remote_peer.interested)
        count++;
    return count;
  }

  void leech() throws SimgridException
  {
    double next_choked_update = Engine.get_clock() + app_bittorrent.UPDATE_CHOKED_INTERVAL;
    Engine.debug("Start downloading.");

    /* Send a "handshake" message to all the peers it got (since it couldn't have gotten more than 50 peers) */
    sendHandshakeToAllPeers();
    Engine.debug("Starting main leech loop listening on mailbox: %s", mailbox.get_name());

    while (Engine.get_clock() < deadline && countPieces(bitfield_) < FILE_PIECES) {
      if (comm_received == null) {
        comm_received = mailbox.get_async();
      }
      if (comm_received.test()) {
        message = (Message)comm_received.get_payload();
        handleMessage();
        message       = null;
        comm_received = null;
      } else {
        /* We don't execute the choke algorithm if we don't already have a piece */
        if (Engine.get_clock() >= next_choked_update && countPieces(bitfield_) > 0) {
          updateChokedPeers();
          next_choked_update += app_bittorrent.UPDATE_CHOKED_INTERVAL;
        } else {
          this.sleep_for(SLEEP_DURATION);
        }
      }
    }
    if (hasFinished())
      Engine.debug("%d becomes a seeder", id);
  }

  void seed() throws SimgridException
  {
    double next_choked_update = Engine.get_clock() + app_bittorrent.UPDATE_CHOKED_INTERVAL;
    Engine.debug("Start seeding.");
    /* start the main seed loop */
    while (Engine.get_clock() < deadline) {
      if (comm_received == null) {
        comm_received = mailbox.get_async();
      }
      if (comm_received.test()) {
        message = (Message)comm_received.get_payload();
        handleMessage();
        message       = null;
        comm_received = null;
      } else {
        if (Engine.get_clock() >= next_choked_update) {
          updateChokedPeers();
          // TODO: Change the choked peer algorithm when seeding.
          next_choked_update += app_bittorrent.UPDATE_CHOKED_INTERVAL;
        } else {
          this.sleep_for(SLEEP_DURATION);
        }
      }
    }
  }

  void updateActivePeersSet(Connection remote_peer)
  {
    if (remote_peer.interested && !remote_peer.choked_upload)
      active_peers.add(remote_peer);
    else
      active_peers.remove(remote_peer);
  }

  void handleMessage()
  {
    Engine.debug("Received a %s message from %s", message_name(message.type), message.return_mailbox.get_name());

    Connection remote_peer = connected_peers.get(message.peer_id);
    if (remote_peer == null && message.type != MessageType.HANDSHAKE)
      Engine.die("The impossible did happened: A not-in-our-list peer sent us a message.");

    switch (message.type) {
      case HANDSHAKE:
        /* Check if the peer is in our connection list. */
        if (remote_peer == null) {
          Engine.debug("This peer %d was unknown, answer to its handshake", message.peer_id);
          remote_peer = new Connection(message.peer_id, this.get_engine());
          connected_peers.put(message.peer_id, remote_peer);
          sendMessage(message.return_mailbox, MessageType.HANDSHAKE, message_size(MessageType.HANDSHAKE));
        }
        /* Send our bitfield to the peer */
        sendBitfield(message.return_mailbox);
        break;
      case BITFIELD:
        /* Update the pieces list */
        updatePiecesCountFromBitfield(message.bitfield);
        /* Store the bitfield */
        remote_peer.bitfield = message.bitfield;
        if (isInterestedBy(remote_peer)) {
          remote_peer.am_interested = true;
          sendMessage(message.return_mailbox, MessageType.INTERESTED, message_size(MessageType.INTERESTED));
        }
        break;
      case INTERESTED:
        /* Update the interested state of the peer. */
        remote_peer.interested = true;
        updateActivePeersSet(remote_peer);
        break;
      case NOTINTERESTED:
        remote_peer.interested = false;
        updateActivePeersSet(remote_peer);
        break;
      case UNCHOKE:
        remote_peer.choked_download = false;
        /* Send requests to the peer, since it has unchoked us */
        if (remote_peer.am_interested)
          requestNewPieceTo(remote_peer);
        break;
      case CHOKE:
        remote_peer.choked_download = true;
        if (remote_peer.current_piece != -1)
          removeCurrentPiece(remote_peer, remote_peer.current_piece);
        break;
      case HAVE:
        Engine.debug("\t for piece %d", message.piece);
        remote_peer.bitfield = remote_peer.bitfield | (1 << message.piece);
        pieces_count[message.piece]++;
        /* If the piece is in our pieces, we tell the peer that we are interested. */
        if (!remote_peer.am_interested && hasNotPiece(message.piece)) {
          remote_peer.am_interested = true;
          sendMessage(message.return_mailbox, MessageType.INTERESTED, message_size(MessageType.INTERESTED));
          if (!remote_peer.choked_download)
            requestNewPieceTo(remote_peer);
        }
        break;
      case REQUEST:
        if (!remote_peer.choked_upload) {
          Engine.debug("\t for piece %d (%d,%d)", message.piece, message.block_index,
                       message.block_index + message.block_length);
          if (!hasNotPiece(message.piece)) {
            sendPiece(message.return_mailbox, message.piece, message.block_index, message.block_length);
          }
        } else {
          Engine.debug("\t for piece %d but he is choked.", message.peer_id);
        }
        break;
      case PIECE:
        Engine.debug(" \t for piece %d (%d,%d)", message.piece, message.block_index,
                     message.block_index + message.block_length);
        // TODO: Execute a computation.
        if (hasNotPiece(message.piece)) {
          updateBitfieldBlocks(message.piece, message.block_index, message.block_length);
          if (hasCompletedPiece(message.piece)) {
            /* Removing the piece from our piece list */
            removeCurrentPiece(remote_peer, message.piece);
            /* Setting the fact that we have the piece */
            bitfield_ = bitfield_ | (1 << message.piece);
            Engine.debug("My status is now %s", getStatus());
            /* Sending the information to all the peers we are connected to */
            sendHaveToAllPeers(message.piece);
            /* sending UNINTERESTED to peers that do not have what we want. */
            updateInterestedAfterReceive();
          } else {                                     // piece not completed
            sendRequestTo(remote_peer, message.piece); // ask for the next block
          }
        } else {
          Engine.debug("However, we already have it");
          requestNewPieceTo(remote_peer);
        }
        break;
      case CANCEL:
        break;
      default:
        throw new IllegalStateException("Impossible message type");
    }
    /* Update the peer speed. */
    if (remote_peer != null) {
      remote_peer.addSpeedValue(1.0 / (Engine.get_clock() - begin_receive_time));
    }
    begin_receive_time = Engine.get_clock();
  }

  /** Selects the appropriate piece to download and requests it to the remote_peer */
  void requestNewPieceTo(Connection remote_peer)
  {
    int piece = selectPieceToDownload(remote_peer);
    if (piece != -1) {
      current_pieces |= (1 << piece);
      sendRequestTo(remote_peer, piece);
    }
  }

  void removeCurrentPiece(Connection remote_peer, int current_piece)
  {
    current_pieces &= ~(1 << current_piece);
    remote_peer.current_piece = -1;
  }

  /**
   * @brief Update "interested" state of peers: send "not interested" to peers that don't have any more pieces we
   * want.
   */
  void updateInterestedAfterReceive()
  {
    for (Connection remote_peer : connected_peers.values()) {
      if (remote_peer.am_interested) {
        boolean interested = false;
        /* Check if the peer still has a piece we want. */
        for (int i = 0; i < FILE_PIECES; i++)
          if (remotePeerHasMissingPiece(remote_peer, i)) {
            interested = true;
            break;
          }

        if (!interested) { // no more piece to download from connection
          remote_peer.am_interested = false;
          sendMessage(remote_peer.mailbox, MessageType.NOTINTERESTED, message_size(MessageType.NOTINTERESTED));
        }
      }
    }
  }

  void updateBitfieldBlocks(int piece, int block_index, int block_length)
  {
    for (int i = block_index; i < (block_index + block_length); i++)
      bitfield_blocks |= (1L << (piece * PIECES_BLOCKS + i));
  }

  boolean hasCompletedPiece(int piece)
  {
    for (int i = 0; i < PIECES_BLOCKS; i++)
      if ((bitfield_blocks & (1L << (piece * PIECES_BLOCKS + i))) == 0)
        return false;
    return true;
  }

  int getFirstMissingBlockFrom(int piece)
  {
    for (int i = 0; i < PIECES_BLOCKS; i++)
      if ((bitfield_blocks & (1L << (piece * PIECES_BLOCKS + i))) == 0)
        return i;
    return -1;
  }

  /** Returns a piece that is partially downloaded and stored by the remote peer if any -1 otherwise. */
  int partiallyDownloadedPiece(Connection remote_peer)
  {
    for (int i = 0; i < FILE_PIECES; i++)
      if (remotePeerHasMissingPiece(remote_peer, i) && isNotDownloadingPiece(i) && getFirstMissingBlockFrom(i) > 0)
        return i;
    return -1;
  }

  /**
   * @brief Return the piece to be downloaded
   * There are two cases (as described in "Bittorrent Architecture Protocol", Ryan Toole :
   * If a piece is partially downloaded, this piece will be selected prioritarily
   * If the peer has strictly less than 4 pieces, he chooses a piece at random.
   * If the peer has more than pieces, he downloads the pieces that are the less replicated (rarest policy).
   * If all pieces have been downloaded or requested, we select a random requested piece (endgame mode).
   * @param remote_peer: information about the connection
   * @return the piece to download if possible. -1 otherwise
   */
  int selectPieceToDownload(Connection remote_peer)
  {
    int piece = partiallyDownloadedPiece(remote_peer);
    // strict priority policy
    if (piece != -1)
      return piece;

    // end game mode
    if (countPieces(current_pieces) >= (FILE_PIECES - countPieces(bitfield_)) && isInterestedBy(remote_peer)) {
      int nb_interesting_pieces = 0;
      // compute the number of interesting pieces
      for (int i = 0; i < FILE_PIECES; i++)
        if (remotePeerHasMissingPiece(remote_peer, i))
          nb_interesting_pieces++;

      // get a random interesting piece
      int random_piece_index = random.nextInt(nb_interesting_pieces);
      int current_index      = 0;
      for (int i = 0; i < FILE_PIECES; i++) {
        if (remotePeerHasMissingPiece(remote_peer, i)) {
          if (random_piece_index == current_index) {
            piece = i;
            break;
          }
          current_index++;
        }
      }
      return piece;
    }
    // Random first policy
    if (countPieces(bitfield_) < 4 && isInterestedByFree(remote_peer)) {
      int nb_interesting_pieces = 0;
      // compute the number of interesting pieces
      for (int i = 0; i < FILE_PIECES; i++)
        if (remotePeerHasMissingPiece(remote_peer, i) && isNotDownloadingPiece(i))
          nb_interesting_pieces++;
      // get a random interesting piece
      int random_piece_index = random.nextInt(nb_interesting_pieces);
      int current_index      = 0;
      for (int i = 0; i < FILE_PIECES; i++) {
        if (remotePeerHasMissingPiece(remote_peer, i) && isNotDownloadingPiece(i)) {
          if (random_piece_index == current_index) {
            piece = i;
            break;
          }
          current_index++;
        }
      }
      return piece;
    } else { // Rarest first policy
      short min         = Short.MAX_VALUE;
      int nb_min_pieces = 0;
      int current_index = 0;
      // compute the smallest number of copies of available pieces
      for (int i = 0; i < FILE_PIECES; i++) {
        if (pieces_count[i] < min && remotePeerHasMissingPiece(remote_peer, i) && isNotDownloadingPiece(i))
          min = pieces_count[i];
      }

      // compute the number of rarest pieces
      for (int i = 0; i < FILE_PIECES; i++)
        if (pieces_count[i] == min && remotePeerHasMissingPiece(remote_peer, i) && isNotDownloadingPiece(i))
          nb_min_pieces++;

      // get a random rarest piece
      int random_rarest_index = 0;
      if (nb_min_pieces > 0) {
        random_rarest_index = random.nextInt(nb_min_pieces);
      }
      for (int i = 0; i < FILE_PIECES; i++)
        if (pieces_count[i] == min && remotePeerHasMissingPiece(remote_peer, i) && isNotDownloadingPiece(i)) {
          if (random_rarest_index == current_index) {
            piece = i;
            break;
          }
          current_index++;
        }

      return piece;
    }
  }

  void updateChokedPeers()
  {
    if (nbInterestedPeers() == 0)
      return;
    Engine.debug("(%d) update_choked peers %d active peers", id, active_peers.size());
    // update the current round
    round_                 = (round_ + 1) % 3;
    Connection chosen_peer = null;
    // select first active peer and remove it from the set
    Connection choked_peer;
    if (active_peers.isEmpty()) {
      choked_peer = null;
    } else {
      choked_peer = active_peers.iterator().next();
      active_peers.remove(choked_peer);
    }

    /* If we are currently seeding, we unchoke the peer which has been unchoked the last time. */
    if (hasFinished()) {
      double unchoke_time = Engine.get_clock() + 1;
      for (Connection remote_peer : connected_peers.values()) {
        if (remote_peer.last_unchoke < unchoke_time && remote_peer.interested && remote_peer.choked_upload) {
          unchoke_time = remote_peer.last_unchoke;
          chosen_peer  = remote_peer;
        }
      }
    } else {
      // Random optimistic unchoking
      if (round_ == 0) {
        List<Connection> peers_list = new ArrayList<>(connected_peers.values());
        int j                       = 0;
        do {
          // We choose a random peer to unchoke.
          chosen_peer = peers_list.get(random.nextInt(peers_list.size()));
          if (!chosen_peer.interested || !chosen_peer.choked_upload)
            chosen_peer = null;
          else
            Engine.debug("Nothing to do, keep going");
          j++;
        } while (chosen_peer == null && j < app_bittorrent.MAXIMUM_PEERS);
      } else {
        // Use the "fastest download" policy.
        double fastest_speed = 0.0;
        for (Connection remote_peer : connected_peers.values()) {
          if (remote_peer.peer_speed > fastest_speed && remote_peer.choked_upload && remote_peer.interested) {
            fastest_speed = remote_peer.peer_speed;
            chosen_peer   = remote_peer;
          }
        }
      }
    }

    if (chosen_peer != null)
      Engine.debug("(%d) update_choked peers unchoked (%d) ; int (%b) ; choked (%b) ", id, chosen_peer.id,
                   chosen_peer.interested, chosen_peer.choked_upload);

    if (choked_peer != chosen_peer) {
      if (choked_peer != null) {
        choked_peer.choked_upload = true;
        updateActivePeersSet(choked_peer);
        Engine.debug("(%d) Sending a CHOKE to %d", id, choked_peer.id);
        sendMessage(choked_peer.mailbox, MessageType.CHOKE, message_size(MessageType.CHOKE));
      }
      if (chosen_peer != null) {
        chosen_peer.choked_upload = false;
        active_peers.add(chosen_peer);
        chosen_peer.last_unchoke = Engine.get_clock();
        Engine.debug("(%d) Sending a UNCHOKE to %d", id, chosen_peer.id);
        updateActivePeersSet(chosen_peer);
        sendMessage(chosen_peer.mailbox, MessageType.UNCHOKE, message_size(MessageType.UNCHOKE));
      }
    }
  }
}

/* ------------------------------------------------------------------------ */
/* The main() function                                                      */
/* ------------------------------------------------------------------------ */

public class app_bittorrent {
  static final String TRACKER_MAILBOX = "tracker_mailbox";
  /** Max number of peers sent by the tracker to clients */
  static final int MAXIMUM_PEERS = 50;
  /** Interval of time where the peer should send a request to the tracker */
  static final int TRACKER_QUERY_INTERVAL = 1000;
  /** Communication size for a message to the tracker */
  static final int TRACKER_COMM_SIZE    = 1;
  static final double GET_PEERS_TIMEOUT = 10000.0;
  /** Number of peers that can be unchoked at a given time */
  static final int MAX_UNCHOKED_PEERS = 4;
  /** Interval between each update of the choked peers */
  static final int UPDATE_CHOKED_INTERVAL = 30;

  public static void main(String[] args)
  {
    Engine e = new Engine(args);

    /* Check the arguments */
    if (args.length < 2)
      Engine.die("Usage: app_bittorrent platform_file deployment_file");

    e.load_platform(args[0]);
    e.load_deployment(args[1]);

    e.run();

    // The following call is useless in your code, but our continuous integration uses it to track memleaks
    e.force_garbage_collection();
  }
}
