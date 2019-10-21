/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <climits>

#include "s4u-peer.hpp"
#include "s4u-tracker.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_bt_peer, "Messages specific for the peers");

/*
 * User parameters for transferred file data. For the test, the default values are :
 * File size: 10 pieces * 5 blocks/piece * 16384 bytes/block = 819200 bytes
 */
constexpr unsigned long FILE_PIECES   = 10UL;
constexpr unsigned long PIECES_BLOCKS = 5UL;
constexpr int BLOCK_SIZE              = 16384;

/** Number of blocks asked by each request */
constexpr unsigned long BLOCKS_REQUESTED = 2UL;

constexpr bool ENABLE_END_GAME_MODE = true;
constexpr double SLEEP_DURATION     = 1.0;
#define BITS_TO_BYTES(x) (((x) / 8 + (x) % 8) ? 1 : 0)

Peer::Peer(std::vector<std::string> args)
{
  // Check arguments
  xbt_assert(args.size() == 3 || args.size() == 4, "Wrong number of arguments");
  try {
    id       = std::stoi(args[1]);
    mailbox_ = simgrid::s4u::Mailbox::by_name(std::to_string(id));
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument("Invalid ID:" + args[1]);
  }

  try {
    deadline = std::stod(args[2]);
  } catch (const std::invalid_argument&) {
    throw std::invalid_argument("Invalid deadline:" + args[2]);
  }
  xbt_assert(deadline > 0, "Wrong deadline supplied");

  if (args.size() == 4 && args[3] == "1") {
    bitfield_       = (1U << FILE_PIECES) - 1U;
    bitfield_blocks = (1ULL << (FILE_PIECES * PIECES_BLOCKS)) - 1ULL;
  }
  pieces_count.resize(FILE_PIECES);

  XBT_INFO("Hi, I'm joining the network with id %d", id);
}

/** Peer main function */
void Peer::operator()()
{
  // Getting peer data from the tracker.
  if (getPeersFromTracker()) {
    XBT_DEBUG("Got %zu peers from the tracker. Current status is: %s", connected_peers.size(), getStatus().c_str());
    begin_receive_time = simgrid::s4u::Engine::get_clock();
    mailbox_->set_receiver(simgrid::s4u::Actor::self());
    if (hasFinished()) {
      sendHandshakeToAllPeers();
    } else {
      leech();
    }
    seed();
  } else {
    XBT_INFO("Couldn't contact the tracker.");
  }

  XBT_INFO("Here is my current status: %s", getStatus().c_str());
}

bool Peer::getPeersFromTracker()
{
  simgrid::s4u::Mailbox* tracker_mailbox = simgrid::s4u::Mailbox::by_name(TRACKER_MAILBOX);
  // Build the task to send to the tracker
  TrackerQuery* peer_request = new TrackerQuery(id, mailbox_);
  try {
    XBT_DEBUG("Sending a peer request to the tracker.");
    tracker_mailbox->put(peer_request, TRACKER_COMM_SIZE, GET_PEERS_TIMEOUT);
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Timeout expired when requesting peers to tracker");
    delete peer_request;
    return false;
  }

  try {
    TrackerAnswer* answer = static_cast<TrackerAnswer*>(mailbox_->get(GET_PEERS_TIMEOUT));
    // Add the peers the tracker gave us to our peer list.
    for (auto const& peer_id : answer->getPeers())
      if (id != peer_id)
        connected_peers.emplace(peer_id, Connection(peer_id));
    delete answer;
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Timeout expired when requesting peers to tracker");
    return false;
  }
  return true;
}

void Peer::sendHandshakeToAllPeers()
{
  for (auto const& kv : connected_peers) {
    const Connection& remote_peer = kv.second;
    Message* handshake      = new Message(MESSAGE_HANDSHAKE, id, mailbox_);
    remote_peer.mailbox_->put_init(handshake, MESSAGE_HANDSHAKE_SIZE)->detach();
    XBT_DEBUG("Sending a HANDSHAKE to %d", remote_peer.id);
  }
}

void Peer::sendMessage(simgrid::s4u::Mailbox* mailbox, e_message_type type, uint64_t size)
{
  const char* type_names[6] = {"HANDSHAKE", "CHOKE", "UNCHOKE", "INTERESTED", "NOTINTERESTED", "CANCEL"};
  XBT_DEBUG("Sending %s to %s", type_names[type], mailbox->get_cname());
  mailbox->put_init(new Message(type, id, bitfield_, mailbox_), size)->detach();
}

void Peer::sendBitfield(simgrid::s4u::Mailbox* mailbox)
{
  XBT_DEBUG("Sending a BITFIELD to %s", mailbox->get_cname());
  mailbox
      ->put_init(new Message(MESSAGE_BITFIELD, id, bitfield_, mailbox_),
                 MESSAGE_BITFIELD_SIZE + BITS_TO_BYTES(FILE_PIECES))
      ->detach();
}

void Peer::sendPiece(simgrid::s4u::Mailbox* mailbox, unsigned int piece, int block_index, int block_length)
{
  xbt_assert(not hasNotPiece(piece), "Tried to send a unavailable piece.");
  XBT_DEBUG("Sending the PIECE %u (%d,%d) to %s", piece, block_index, block_length, mailbox->get_cname());
  mailbox->put_init(new Message(MESSAGE_PIECE, id, mailbox_, piece, block_index, block_length), BLOCK_SIZE)->detach();
}

void Peer::sendHaveToAllPeers(unsigned int piece)
{
  XBT_DEBUG("Sending HAVE message to all my peers");
  for (auto const& kv : connected_peers) {
    const Connection& remote_peer = kv.second;
    remote_peer.mailbox_->put_init(new Message(MESSAGE_HAVE, id, mailbox_, piece), MESSAGE_HAVE_SIZE)->detach();
  }
}

void Peer::sendRequestTo(Connection* remote_peer, unsigned int piece)
{
  remote_peer->current_piece = piece;
  xbt_assert(remote_peer->hasPiece(piece));
  int block_index = getFirstMissingBlockFrom(piece);
  if (block_index != -1) {
    int block_length = std::min(BLOCKS_REQUESTED, PIECES_BLOCKS - block_index);
    XBT_DEBUG("Sending a REQUEST to %s for piece %u (%d,%d)", remote_peer->mailbox_->get_cname(), piece, block_index,
              block_length);
    remote_peer->mailbox_
        ->put_init(new Message(MESSAGE_REQUEST, id, mailbox_, piece, block_index, block_length), MESSAGE_REQUEST_SIZE)
        ->detach();
  }
}

std::string Peer::getStatus()
{
  std::string res = std::string("");
  for (int i = FILE_PIECES - 1; i >= 0; i--)
    res = std::string((bitfield_ & (1U << i)) ? "1" : "0") + res;
  return res;
}

bool Peer::hasFinished()
{
  return bitfield_ == (1U << FILE_PIECES) - 1U;
}

/** Indicates if the remote peer has a piece not stored by the local peer */
bool Peer::isInterestedBy(Connection* remote_peer)
{
  return remote_peer->bitfield & (bitfield_ ^ ((1 << FILE_PIECES) - 1));
}

bool Peer::isInterestedByFree(Connection* remote_peer)
{
  for (unsigned int i = 0; i < FILE_PIECES; i++)
    if (hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i))
      return true;
  return false;
}

void Peer::updatePiecesCountFromBitfield(unsigned int bitfield)
{
  for (unsigned int i = 0; i < FILE_PIECES; i++)
    if (bitfield & (1U << i))
      pieces_count[i]++;
}

unsigned int Peer::countPieces(unsigned int bitfield)
{
  unsigned int count = 0U;
  unsigned int n     = bitfield;
  while (n) {
    count += n & 1U;
    n >>= 1U;
  }
  return count;
}

int Peer::nbInterestedPeers()
{
  int nb = 0;
  for (auto const& kv : connected_peers)
    if (kv.second.interested)
      nb++;
  return nb;
}

void Peer::leech()
{
  double next_choked_update = simgrid::s4u::Engine::get_clock() + UPDATE_CHOKED_INTERVAL;
  XBT_DEBUG("Start downloading.");

  /* Send a "handshake" message to all the peers it got (since it couldn't have gotten more than 50 peers) */
  sendHandshakeToAllPeers();
  XBT_DEBUG("Starting main leech loop listening on mailbox: %s", mailbox_->get_cname());

  void* data = nullptr;
  while (simgrid::s4u::Engine::get_clock() < deadline && countPieces(bitfield_) < FILE_PIECES) {
    if (comm_received == nullptr) {
      comm_received = mailbox_->get_async(&data);
    }
    if (comm_received->test()) {
      message = static_cast<Message*>(data);
      handleMessage();
      delete message;
      comm_received = nullptr;
    } else {
      // We don't execute the choke algorithm if we don't already have a piece
      if (simgrid::s4u::Engine::get_clock() >= next_choked_update && countPieces(bitfield_) > 0) {
        updateChokedPeers();
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        simgrid::s4u::this_actor::sleep_for(SLEEP_DURATION);
      }
    }
  }
  if (hasFinished())
    XBT_DEBUG("%d becomes a seeder", id);
}

void Peer::seed()
{
  double next_choked_update = simgrid::s4u::Engine::get_clock() + UPDATE_CHOKED_INTERVAL;
  XBT_DEBUG("Start seeding.");
  // start the main seed loop
  void* data = nullptr;
  while (simgrid::s4u::Engine::get_clock() < deadline) {
    if (comm_received == nullptr) {
      comm_received = mailbox_->get_async(&data);
    }
    if (comm_received->test()) {
      message = static_cast<Message*>(data);
      handleMessage();
      delete message;
      comm_received = nullptr;
    } else {
      if (simgrid::s4u::Engine::get_clock() >= next_choked_update) {
        updateChokedPeers();
        // TODO: Change the choked peer algorithm when seeding.
        next_choked_update += UPDATE_CHOKED_INTERVAL;
      } else {
        simgrid::s4u::this_actor::sleep_for(SLEEP_DURATION);
      }
    }
  }
}

void Peer::updateActivePeersSet(Connection* remote_peer)
{
  if (remote_peer->interested && not remote_peer->choked_upload)
    active_peers.insert(remote_peer);
  else
    active_peers.erase(remote_peer);
}

void Peer::handleMessage()
{
  const char* type_names[10] = {"HANDSHAKE", "CHOKE",    "UNCHOKE", "INTERESTED", "NOTINTERESTED",
                                "HAVE",      "BITFIELD", "REQUEST", "PIECE",      "CANCEL"};

  XBT_DEBUG("Received a %s message from %s", type_names[message->type], message->return_mailbox->get_cname());

  auto known_peer         = connected_peers.find(message->peer_id);
  Connection* remote_peer = (known_peer == connected_peers.end()) ? nullptr : &known_peer->second;
  xbt_assert(remote_peer != nullptr || message->type == MESSAGE_HANDSHAKE,
             "The impossible did happened: A not-in-our-list peer sent us a message.");

  switch (message->type) {
    case MESSAGE_HANDSHAKE:
      // Check if the peer is in our connection list.
      if (remote_peer == nullptr) {
        XBT_DEBUG("This peer %d was unknown, answer to its handshake", message->peer_id);
        connected_peers.emplace(message->peer_id, Connection(message->peer_id));
        sendMessage(message->return_mailbox, MESSAGE_HANDSHAKE, MESSAGE_HANDSHAKE_SIZE);
      }
      // Send our bitfield to the peer
      sendBitfield(message->return_mailbox);
      break;
    case MESSAGE_BITFIELD:
      // Update the pieces list
      updatePiecesCountFromBitfield(message->bitfield);
      // Store the bitfield
      remote_peer->bitfield = message->bitfield;
      xbt_assert(not remote_peer->am_interested, "Should not be interested at first");
      if (isInterestedBy(remote_peer)) {
        remote_peer->am_interested = true;
        sendMessage(message->return_mailbox, MESSAGE_INTERESTED, MESSAGE_INTERESTED_SIZE);
      }
      break;
    case MESSAGE_INTERESTED:
      // Update the interested state of the peer.
      remote_peer->interested = true;
      updateActivePeersSet(remote_peer);
      break;
    case MESSAGE_NOTINTERESTED:
      remote_peer->interested = false;
      updateActivePeersSet(remote_peer);
      break;
    case MESSAGE_UNCHOKE:
      xbt_assert(remote_peer->choked_download);
      remote_peer->choked_download = false;
      // Send requests to the peer, since it has unchoked us
      if (remote_peer->am_interested)
        requestNewPieceTo(remote_peer);
      break;
    case MESSAGE_CHOKE:
      xbt_assert(not remote_peer->choked_download);
      remote_peer->choked_download = true;
      if (remote_peer->current_piece != -1)
        removeCurrentPiece(remote_peer, remote_peer->current_piece);
      break;
    case MESSAGE_HAVE:
      XBT_DEBUG("\t for piece %d", message->piece);
      xbt_assert((message->piece >= 0 && static_cast<unsigned int>(message->piece) < FILE_PIECES),
                 "Wrong HAVE message received");
      remote_peer->bitfield = remote_peer->bitfield | (1U << static_cast<unsigned int>(message->piece));
      pieces_count[message->piece]++;
      // If the piece is in our pieces, we tell the peer that we are interested.
      if (not remote_peer->am_interested && hasNotPiece(message->piece)) {
        remote_peer->am_interested = true;
        sendMessage(message->return_mailbox, MESSAGE_INTERESTED, MESSAGE_INTERESTED_SIZE);
        if (not remote_peer->choked_download)
          requestNewPieceTo(remote_peer);
      }
      break;
    case MESSAGE_REQUEST:
      xbt_assert(remote_peer->interested);
      xbt_assert((message->piece >= 0 && static_cast<unsigned int>(message->piece) < FILE_PIECES),
                 "Wrong HAVE message received");
      if (not remote_peer->choked_upload) {
        XBT_DEBUG("\t for piece %d (%d,%d)", message->piece, message->block_index,
                  message->block_index + message->block_length);
        if (not hasNotPiece(message->piece)) {
          sendPiece(message->return_mailbox, message->piece, message->block_index, message->block_length);
        }
      } else {
        XBT_DEBUG("\t for piece %d but he is choked.", message->peer_id);
      }
      break;
    case MESSAGE_PIECE:
      XBT_DEBUG(" \t for piece %d (%d,%d)", message->piece, message->block_index,
                message->block_index + message->block_length);
      xbt_assert(not remote_peer->choked_download);
      xbt_assert(remote_peer->am_interested || ENABLE_END_GAME_MODE,
                 "Can't received a piece if I'm not interested without end-game mode!"
                 "piece (%d) bitfield (%u) remote bitfield (%u)",
                 message->piece, bitfield_, remote_peer->bitfield);
      xbt_assert(not remote_peer->choked_download, "Can't received a piece if I'm choked !");
      xbt_assert((message->piece >= 0 && static_cast<unsigned int>(message->piece) < FILE_PIECES),
                 "Wrong piece received");
      // TODO: Execute a computation.
      if (hasNotPiece(static_cast<unsigned int>(message->piece))) {
        updateBitfieldBlocks(message->piece, message->block_index, message->block_length);
        if (hasCompletedPiece(static_cast<unsigned int>(message->piece))) {
          // Removing the piece from our piece list
          removeCurrentPiece(remote_peer, message->piece);
          // Setting the fact that we have the piece
          bitfield_ = bitfield_ | (1U << static_cast<unsigned int>(message->piece));
          XBT_DEBUG("My status is now %s", getStatus().c_str());
          // Sending the information to all the peers we are connected to
          sendHaveToAllPeers(message->piece);
          // sending UNINTERESTED to peers that do not have what we want.
          updateInterestedAfterReceive();
        } else {                                      // piece not completed
          sendRequestTo(remote_peer, message->piece); // ask for the next block
        }
      } else {
        XBT_DEBUG("However, we already have it");
        xbt_assert(ENABLE_END_GAME_MODE, "Should not happen because we don't use end game mode !");
        requestNewPieceTo(remote_peer);
      }
      break;
    case MESSAGE_CANCEL:
      break;
    default:
      THROW_IMPOSSIBLE;
  }
  // Update the peer speed.
  if (remote_peer) {
    remote_peer->addSpeedValue(1.0 / (simgrid::s4u::Engine::get_clock() - begin_receive_time));
  }
  begin_receive_time = simgrid::s4u::Engine::get_clock();
}

/** Selects the appropriate piece to download and requests it to the remote_peer */
void Peer::requestNewPieceTo(Connection* remote_peer)
{
  int piece = selectPieceToDownload(remote_peer);
  if (piece != -1) {
    current_pieces |= (1U << (unsigned int)piece);
    sendRequestTo(remote_peer, piece);
  }
}

void Peer::removeCurrentPiece(Connection* remote_peer, unsigned int current_piece)
{
  current_pieces &= ~(1U << current_piece);
  remote_peer->current_piece = -1;
}

/** @brief Return the piece to be downloaded
 * There are two cases (as described in "Bittorrent Architecture Protocol", Ryan Toole :
 * If a piece is partially downloaded, this piece will be selected prioritarily
 * If the peer has strictly less than 4 pieces, he chooses a piece at random.
 * If the peer has more than pieces, he downloads the pieces that are the less replicated (rarest policy).
 * If all pieces have been downloaded or requested, we select a random requested piece (endgame mode).
 * @param remote_peer: information about the connection
 * @return the piece to download if possible. -1 otherwise
 */
int Peer::selectPieceToDownload(Connection* remote_peer)
{
  int piece = partiallyDownloadedPiece(remote_peer);
  // strict priority policy
  if (piece != -1)
    return piece;

  // end game mode
  if (countPieces(current_pieces) >= (FILE_PIECES - countPieces(bitfield_)) && isInterestedBy(remote_peer)) {
    if (not ENABLE_END_GAME_MODE)
      return -1;
    int nb_interesting_pieces = 0;
    // compute the number of interesting pieces
    for (unsigned int i = 0; i < FILE_PIECES; i++)
      if (hasNotPiece(i) && remote_peer->hasPiece(i))
        nb_interesting_pieces++;

    xbt_assert(nb_interesting_pieces != 0);
    // get a random interesting piece
    int random_piece_index = simgrid::xbt::random::uniform_int(0, nb_interesting_pieces - 1);
    int current_index      = 0;
    for (unsigned int i = 0; i < FILE_PIECES; i++) {
      if (hasNotPiece(i) && remote_peer->hasPiece(i)) {
        if (random_piece_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1);
    return piece;
  }
  // Random first policy
  if (countPieces(bitfield_) < 4 && isInterestedByFree(remote_peer)) {
    int nb_interesting_pieces = 0;
    // compute the number of interesting pieces
    for (unsigned int i = 0; i < FILE_PIECES; i++)
      if (hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i))
        nb_interesting_pieces++;
    xbt_assert(nb_interesting_pieces != 0);
    // get a random interesting piece
    int random_piece_index = simgrid::xbt::random::uniform_int(0, nb_interesting_pieces - 1);
    int current_index      = 0;
    for (unsigned int i = 0; i < FILE_PIECES; i++) {
      if (hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i)) {
        if (random_piece_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }
    }
    xbt_assert(piece != -1);
    return piece;
  } else { // Rarest first policy
    short min         = SHRT_MAX;
    int nb_min_pieces = 0;
    int current_index = 0;
    // compute the smallest number of copies of available pieces
    for (unsigned int i = 0; i < FILE_PIECES; i++) {
      if (pieces_count[i] < min && hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i))
        min = pieces_count[i];
    }

    xbt_assert(min != SHRT_MAX || not isInterestedByFree(remote_peer));
    // compute the number of rarest pieces
    for (unsigned int i = 0; i < FILE_PIECES; i++)
      if (pieces_count[i] == min && hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i))
        nb_min_pieces++;

    xbt_assert(nb_min_pieces != 0 || not isInterestedByFree(remote_peer));
    // get a random rarest piece
    int random_rarest_index = 0;
    if (nb_min_pieces > 0) {
      random_rarest_index = simgrid::xbt::random::uniform_int(0, nb_min_pieces - 1);
    }
    for (unsigned int i = 0; i < FILE_PIECES; i++)
      if (pieces_count[i] == min && hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i)) {
        if (random_rarest_index == current_index) {
          piece = i;
          break;
        }
        current_index++;
      }

    xbt_assert(piece != -1 || not isInterestedByFree(remote_peer));
    return piece;
  }
}

void Peer::updateChokedPeers()
{
  if (nbInterestedPeers() == 0)
    return;
  XBT_DEBUG("(%d) update_choked peers %zu active peers", id, active_peers.size());
  // update the current round
  round_                  = (round_ + 1) % 3;
  Connection* chosen_peer = nullptr;
  // select first active peer and remove it from the set
  Connection* choked_peer;
  if (active_peers.empty()) {
    choked_peer = nullptr;
  } else {
    choked_peer = *active_peers.begin();
    active_peers.erase(choked_peer);
  }

  /**If we are currently seeding, we unchoke the peer which has been unchoked the last time.*/
  if (hasFinished()) {
    double unchoke_time = simgrid::s4u::Engine::get_clock() + 1;
    for (auto& kv : connected_peers) {
      Connection& remote_peer = kv.second;
      if (remote_peer.last_unchoke < unchoke_time && remote_peer.interested && remote_peer.choked_upload) {
        unchoke_time = remote_peer.last_unchoke;
        chosen_peer  = &remote_peer;
      }
    }
  } else {
    // Random optimistic unchoking
    if (round_ == 0) {
      int j = 0;
      do {
        // We choose a random peer to unchoke.
        std::unordered_map<int, Connection>::iterator chosen_peer_it = connected_peers.begin();
        std::advance(chosen_peer_it, simgrid::xbt::random::uniform_int(0, connected_peers.size() - 1));
        chosen_peer = &chosen_peer_it->second;
        if (not chosen_peer->interested || not chosen_peer->choked_upload)
          chosen_peer = nullptr;
        else
          XBT_DEBUG("Nothing to do, keep going");
        j++;
      } while (chosen_peer == nullptr && j < MAXIMUM_PEERS);
    } else {
      // Use the "fastest download" policy.
      double fastest_speed = 0.0;
      for (auto& kv : connected_peers) {
        Connection& remote_peer = kv.second;
        if (remote_peer.peer_speed > fastest_speed && remote_peer.choked_upload && remote_peer.interested) {
          fastest_speed = remote_peer.peer_speed;
          chosen_peer   = &remote_peer;
        }
      }
    }
  }

  if (chosen_peer != nullptr)
    XBT_DEBUG("(%d) update_choked peers unchoked (%d) ; int (%d) ; choked (%d) ", id, chosen_peer->id,
              chosen_peer->interested, chosen_peer->choked_upload);

  if (choked_peer != chosen_peer) {
    if (choked_peer != nullptr) {
      xbt_assert(not choked_peer->choked_upload, "Tries to choked a choked peer");
      choked_peer->choked_upload = true;
      updateActivePeersSet(choked_peer);
      XBT_DEBUG("(%d) Sending a CHOKE to %d", id, choked_peer->id);
      sendMessage(choked_peer->mailbox_, MESSAGE_CHOKE, MESSAGE_CHOKE_SIZE);
    }
    if (chosen_peer != nullptr) {
      xbt_assert((chosen_peer->choked_upload), "Tries to unchoked an unchoked peer");
      chosen_peer->choked_upload = false;
      active_peers.insert(chosen_peer);
      chosen_peer->last_unchoke = simgrid::s4u::Engine::get_clock();
      XBT_DEBUG("(%d) Sending a UNCHOKE to %d", id, chosen_peer->id);
      updateActivePeersSet(chosen_peer);
      sendMessage(chosen_peer->mailbox_, MESSAGE_UNCHOKE, MESSAGE_UNCHOKE_SIZE);
    }
  }
}

/** @brief Update "interested" state of peers: send "not interested" to peers that don't have any more pieces we want.*/
void Peer::updateInterestedAfterReceive()
{
  for (auto& kv : connected_peers) {
    Connection& remote_peer = kv.second;
    if (remote_peer.am_interested) {
      bool interested = false;
      // Check if the peer still has a piece we want.
      for (unsigned int i = 0; i < FILE_PIECES; i++)
        if (hasNotPiece(i) && remote_peer.hasPiece(i)) {
          interested = true;
          break;
        }

      if (not interested) { // no more piece to download from connection
        remote_peer.am_interested = false;
        sendMessage(remote_peer.mailbox_, MESSAGE_NOTINTERESTED, MESSAGE_NOTINTERESTED_SIZE);
      }
    }
  }
}

void Peer::updateBitfieldBlocks(int piece, int block_index, int block_length)
{
  xbt_assert((piece >= 0 && static_cast<unsigned int>(piece) <= FILE_PIECES), "Wrong piece.");
  xbt_assert((block_index >= 0 && static_cast<unsigned int>(block_index) <= PIECES_BLOCKS), "Wrong block : %d.",
             block_index);
  for (int i = block_index; i < (block_index + block_length); i++)
    bitfield_blocks |= (1ULL << static_cast<unsigned int>(piece * PIECES_BLOCKS + i));
}

bool Peer::hasCompletedPiece(unsigned int piece)
{
  for (unsigned int i = 0; i < PIECES_BLOCKS; i++)
    if (not(bitfield_blocks & 1ULL << (piece * PIECES_BLOCKS + i)))
      return false;
  return true;
}

int Peer::getFirstMissingBlockFrom(int piece)
{
  for (unsigned int i = 0; i < PIECES_BLOCKS; i++)
    if (not(bitfield_blocks & 1ULL << (piece * PIECES_BLOCKS + i)))
      return i;
  return -1;
}

/** Returns a piece that is partially downloaded and stored by the remote peer if any -1 otherwise. */
int Peer::partiallyDownloadedPiece(Connection* remote_peer)
{
  for (unsigned int i = 0; i < FILE_PIECES; i++)
    if (hasNotPiece(i) && remote_peer->hasPiece(i) && isNotDownloadingPiece(i) && getFirstMissingBlockFrom(i) > 0)
      return i;
  return -1;
}
