/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-dht-chord.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(s4u_chord);

/* Returns whether an id belongs to the interval [start, end].
 *
 * The parameters are normalized to make sure they are between 0 and nb_keys - 1).
 * 1 belongs to [62, 3]
 * 1 does not belong to [3, 62]
 * 63 belongs to [62, 3]
 * 63 does not belong to [3, 62]
 * 24 belongs to [21, 29]
 * 24 does not belong to [29, 21]
 *
 * @param id id to check
 * @param start lower bound
 * @param end upper bound
 * @return a non-zero value if id in in [start, end]
 */
static int is_in_interval(int id, int start, int end)
{
  int i = id % nb_keys;
  int s = start % nb_keys;
  int e = end % nb_keys;

  // make sure end >= start and id >= start
  if (e < s) {
    e += nb_keys;
  }

  if (i < s) {
    i += nb_keys;
  }

  return i <= e;
}

void ChordMessage::destroy(void* message)
{
  delete static_cast<ChordMessage*>(message);
}

/* Initializes the current node as the first one of the system */
Node::Node(std::vector<std::string> args)
{
  xbt_assert(args.size() == 3 || args.size() == 5, "Wrong number of arguments for this node");

  // initialize my node
  id_                = std::stoi(args[1]);
  mailbox_           = simgrid::s4u::Mailbox::by_name(std::to_string(id_));
  next_finger_to_fix = 0;
  fingers_.resize(nb_bits, id_);

  if (args.size() == 3) { // first ring
    deadline_   = std::stod(args[2]);
    start_time_ = simgrid::s4u::Engine::get_clock();
    XBT_DEBUG("Create a new Chord ring...");
  } else {
    known_id_   = std::stoi(args[2]);
    start_time_ = std::stod(args[3]);
    deadline_   = std::stod(args[4]);
    XBT_DEBUG("Hey! Let's join the system in %f seconds (shall leave at time %f)", start_time_,
              start_time_ + deadline_);
  }
}

/* Makes the current node join the ring, knowing the id of a node already in the ring
 *
 * @param known_id id of a node already in the ring
 * @return true if the join operation succeeded
 *  */

void Node::join(int known_id)
{
  XBT_INFO("Joining the ring with id %d, knowing node %d", id_, known_id);
  setPredecessor(-1); // no predecessor (yet)

  int successor_id = remoteFindSuccessor(known_id, id_);
  if (successor_id == -1) {
    XBT_INFO("Cannot join the ring.");
  } else {
    setFinger(0, successor_id);
    printFingerTable();
    joined = true;
  }
}

/* Makes the current node quit the system */
void Node::leave()
{
  XBT_INFO("Well Guys! I Think it's time for me to leave ;)");
  notifyAndQuit();
  joined = false;
}

/* Notifies the successor and the predecessor of the current node before leaving */
void Node::notifyAndQuit()
{
  // send the PREDECESSOR_LEAVING to our successor
  ChordMessage* pred_msg = new ChordMessage(PREDECESSOR_LEAVING);
  pred_msg->request_id   = pred_id_;
  pred_msg->answer_to    = mailbox_;

  XBT_DEBUG("Sending a 'PREDECESSOR_LEAVING' to my successor %d", fingers_[0]);
  try {
    simgrid::s4u::Mailbox::by_name(std::to_string(fingers_[0]))->put(pred_msg, 10, timeout);
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Timeout expired when sending a 'PREDECESSOR_LEAVING' to my successor %d", fingers_[0]);
    delete pred_msg;
  }

  if (pred_id_ != -1 && pred_id_ != id_) {
    // send the SUCCESSOR_LEAVING to our predecessor (only if I have one that is not me)
    ChordMessage* succ_msg = new ChordMessage(SUCCESSOR_LEAVING);
    succ_msg->request_id   = fingers_[0];
    succ_msg->answer_to    = mailbox_;
    XBT_DEBUG("Sending a 'SUCCESSOR_LEAVING' to my predecessor %d", pred_id_);

    try {
      simgrid::s4u::Mailbox::by_name(std::to_string(pred_id_))->put(succ_msg, 10, timeout);
    } catch (const simgrid::TimeoutException&) {
      XBT_DEBUG("Timeout expired when sending a 'SUCCESSOR_LEAVING' to my predecessor %d", pred_id_);
      delete succ_msg;
    }
  }
}

/* Performs a find successor request to a random id */
void Node::randomLookup()
{
  int res          = id_;
  // std::uniform_int_distribution<int> dist(0, nb_bits - 1);
  // int random_index = dist(generator);
  int random_index = generator() % nb_bits; // ensure reproducibility across platforms
  int random_id    = fingers_[random_index];
  XBT_DEBUG("Making a lookup request for id %d", random_id);
  if (random_id != id_)
    res = findSuccessor(random_id);
  XBT_DEBUG("The successor of node %d is %d", random_id, res);
}

/* Sets a finger of the current node.
 *
 * @param node the current node
 * @param finger_index index of the finger to set (0 to nb_bits - 1)
 * @param id the id to set for this finger
 */
void Node::setFinger(int finger_index, int id)
{
  if (id != fingers_[finger_index]) {
    fingers_[finger_index] = id;
    XBT_VERB("My new finger #%d is %d", finger_index, id);
  }
}

/* Sets the predecessor of the current node.
 * @param id the id to predecessor, or -1 to unset the predecessor
 */
void Node::setPredecessor(int predecessor_id)
{
  if (predecessor_id != pred_id_) {
    pred_id_ = predecessor_id;
    XBT_VERB("My new predecessor is %d", predecessor_id);
  }
}

/** refreshes the finger table of the current node (called periodically) */
void Node::fixFingers()
{
  XBT_DEBUG("Fixing fingers");
  int id = findSuccessor(id_ + (1U << next_finger_to_fix));
  if (id != -1) {
    if (id != fingers_[next_finger_to_fix]) {
      setFinger(next_finger_to_fix, id);
      printFingerTable();
    }
    next_finger_to_fix = (next_finger_to_fix + 1) % nb_bits;
  }
}

/** Displays the finger table of a node. */
void Node::printFingerTable()
{
  if (XBT_LOG_ISENABLED(s4u_chord, xbt_log_priority_verbose)) {
    XBT_VERB("My finger table:");
    XBT_VERB("Start | Succ");
    for (int i = 0; i < nb_bits; i++) {
      XBT_VERB(" %3u  | %3d", (id_ + (1U << i)) % nb_keys, fingers_[i]);
    }

    XBT_VERB("Predecessor: %d", pred_id_);
  }
}

/* checks whether the predecessor has failed (called periodically) */
void Node::checkPredecessor()
{
  XBT_DEBUG("Checking whether my predecessor is alive");
  void* data = nullptr;
  if (pred_id_ == -1)
    return;

  simgrid::s4u::Mailbox* mailbox        = simgrid::s4u::Mailbox::by_name(std::to_string(pred_id_));
  simgrid::s4u::Mailbox* return_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(id_) + "_is_alive");

  ChordMessage* message = new ChordMessage(PREDECESSOR_ALIVE);
  message->request_id   = pred_id_;
  message->answer_to    = return_mailbox;

  XBT_DEBUG("Sending a 'Predecessor Alive' request to my predecessor %d", pred_id_);
  try {
    mailbox->put(message, 10, timeout);
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Failed to send the 'Predecessor Alive' request to %d", pred_id_);
    delete message;
    return;
  }

  // receive the answer
  XBT_DEBUG("Sent 'Predecessor Alive' request to %d, waiting for the answer on my mailbox '%s'", pred_id_,
            message->answer_to->get_cname());
  simgrid::s4u::CommPtr comm = return_mailbox->get_async(&data);

  try {
    comm->wait_for(timeout);
    XBT_DEBUG("Received the answer to my 'Predecessor Alive': my predecessor %d is alive", pred_id_);
    delete static_cast<ChordMessage*>(data);
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Failed to receive the answer to my 'Predecessor Alive' request");
    pred_id_ = -1;
  }
}

/* Asks its predecessor to a remote node
 *
 * @param ask_to the node to ask to
 * @return the id of its predecessor node, or -1 if the request failed (or if the node does not know its predecessor)
 */
int Node::remoteGetPredecessor(int ask_to)
{
  int predecessor_id                      = -1;
  void* data                              = nullptr;
  simgrid::s4u::Mailbox* mailbox          = simgrid::s4u::Mailbox::by_name(std::to_string(ask_to));
  simgrid::s4u::Mailbox* return_mailbox   = simgrid::s4u::Mailbox::by_name(std::to_string(id_) + "_pred");

  ChordMessage* message = new ChordMessage(GET_PREDECESSOR);
  message->request_id   = id_;
  message->answer_to    = return_mailbox;

  // send a "Get Predecessor" request to ask_to_id
  XBT_DEBUG("Sending a 'Get Predecessor' request to %d", ask_to);
  try {
    mailbox->put(message, 10, timeout);
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Failed to send the 'Get Predecessor' request to %d", ask_to);
    delete message;
    return predecessor_id;
  }

  // receive the answer
  XBT_DEBUG("Sent 'Get Predecessor' request to %d, waiting for the answer on my mailbox '%s'", ask_to,
            message->answer_to->get_cname());
  simgrid::s4u::CommPtr comm = return_mailbox->get_async(&data);

  try {
    comm->wait_for(timeout);
    ChordMessage* answer = static_cast<ChordMessage*>(data);
    XBT_DEBUG("Received the answer to my 'Get Predecessor' request: the predecessor of node %d is %d", ask_to,
              answer->answer_id);
    predecessor_id = answer->answer_id;
    delete answer;
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Failed to receive the answer to my 'Get Predecessor' request");
    delete static_cast<ChordMessage*>(data);
  }

  return predecessor_id;
}

/* Returns the closest preceding finger of an id with respect to the finger table of the current node.
 *
 * @param id the id to find
 * @return the closest preceding finger of that id
 */
int Node::closestPrecedingFinger(int id)
{
  for (int i = nb_bits - 1; i >= 0; i--) {
    if (is_in_interval(fingers_[i], id_ + 1, id - 1)) {
      return fingers_[i];
    }
  }
  return id_;
}

/* Makes the current node find the successor node of an id.
 *
 * @param id the id to find
 * @return the id of the successor node, or -1 if the request failed
 */
int Node::findSuccessor(int id)
{
  // is my successor the successor?
  if (is_in_interval(id, id_ + 1, fingers_[0])) {
    return fingers_[0];
  }

  // otherwise, ask the closest preceding finger in my table
  return remoteFindSuccessor(closestPrecedingFinger(id), id);
}

int Node::remoteFindSuccessor(int ask_to, int id)
{
  int successor                           = -1;
  void* data                              = nullptr;
  simgrid::s4u::Mailbox* mailbox          = simgrid::s4u::Mailbox::by_name(std::to_string(ask_to));
  simgrid::s4u::Mailbox* return_mailbox   = simgrid::s4u::Mailbox::by_name(std::to_string(id_) + "_succ");

  ChordMessage* message = new ChordMessage(FIND_SUCCESSOR);
  message->request_id   = id_;
  message->answer_to    = return_mailbox;

  // send a "Find Successor" request to ask_to_id
  XBT_DEBUG("Sending a 'Find Successor' request to %d for id %d", ask_to, id);
  try {
    mailbox->put(message, 10, timeout);
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Failed to send the 'Find Successor' request to %d for id %d", ask_to, id_);
    delete message;
    return successor;
  }
  // receive the answer
  XBT_DEBUG("Sent a 'Find Successor' request to %d for key %d, waiting for the answer", ask_to, id);
  simgrid::s4u::CommPtr comm = return_mailbox->get_async(&data);

  try {
    comm->wait_for(timeout);
    ChordMessage* answer = static_cast<ChordMessage*>(data);
    XBT_DEBUG("Received the answer to my 'Find Successor' request for id %d: the successor of key %d is %d",
              answer->request_id, id_, answer->answer_id);
    successor = answer->answer_id;
    delete answer;
  } catch (const simgrid::TimeoutException&) {
    XBT_DEBUG("Failed to receive the answer to my 'Find Successor' request");
    delete static_cast<ChordMessage*>(data);
  }

  return successor;
}

/* Notifies the current node that its predecessor may have changed. */
void Node::notify(int predecessor_candidate_id)
{
  if (pred_id_ == -1 || is_in_interval(predecessor_candidate_id, pred_id_ + 1, id_ - 1)) {
    setPredecessor(predecessor_candidate_id);
    printFingerTable();
  } else {
    XBT_DEBUG("I don't have to change my predecessor to %d", predecessor_candidate_id);
  }
}

/* Notifies a remote node that its predecessor may have changed. */
void Node::remoteNotify(int notify_id, int predecessor_candidate_id)
{
  ChordMessage* message = new ChordMessage(NOTIFY);
  message->request_id   = predecessor_candidate_id;
  message->answer_to    = nullptr;

  // send a "Notify" request to notify_id
  XBT_DEBUG("Sending a 'Notify' request to %d", notify_id);
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(notify_id));
  mailbox->put_init(message, 10)->detach(ChordMessage::destroy);
}

/* This function is called periodically. It checks the immediate successor of the current node. */
void Node::stabilize()
{
  XBT_DEBUG("Stabilizing node");

  // get the predecessor of my immediate successor
  int candidate_id = pred_id_;
  int successor_id = fingers_[0];
  if (successor_id != id_)
    candidate_id = remoteGetPredecessor(successor_id);

  // this node is a candidate to become my new successor
  if (candidate_id != -1 && is_in_interval(candidate_id, id_ + 1, successor_id - 1)) {
    setFinger(0, candidate_id);
  }
  if (successor_id != id_) {
    remoteNotify(successor_id, id_);
  }
}

/* This function is called when a node receives a message.
 *
 * @param message the message to handle (don't touch it afterward: it will be destroyed, reused or forwarded)
 */
void Node::handleMessage(ChordMessage* message)
{
  switch (message->type) {
  case FIND_SUCCESSOR:
    XBT_DEBUG("Received a 'Find Successor' request from %s for id %d", message->issuer_host_name.c_str(),
        message->request_id);
    // is my successor the successor?
    if (is_in_interval(message->request_id, id_ + 1, fingers_[0])) {
      message->type = FIND_SUCCESSOR_ANSWER;
      message->answer_id = fingers_[0];
      XBT_DEBUG("Sending back a 'Find Successor Answer' to %s (mailbox %s): the successor of %d is %d",
                message->issuer_host_name.c_str(), message->answer_to->get_cname(), message->request_id,
                message->answer_id);
      message->answer_to->put_init(message, 10)->detach(ChordMessage::destroy);
    } else {
      // otherwise, forward the request to the closest preceding finger in my table
      int closest = closestPrecedingFinger(message->request_id);
      XBT_DEBUG("Forwarding the 'Find Successor' request for id %d to my closest preceding finger %d",
          message->request_id, closest);
      simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(closest));
      mailbox->put_init(message, 10)->detach(ChordMessage::destroy);
    }
    break;

  case GET_PREDECESSOR:
    XBT_DEBUG("Receiving a 'Get Predecessor' request from %s", message->issuer_host_name.c_str());
    message->type = GET_PREDECESSOR_ANSWER;
    message->answer_id = pred_id_;
    XBT_DEBUG("Sending back a 'Get Predecessor Answer' to %s via mailbox '%s': my predecessor is %d",
              message->issuer_host_name.c_str(), message->answer_to->get_cname(), message->answer_id);
    message->answer_to->put_init(message, 10)->detach(ChordMessage::destroy);
    break;

  case NOTIFY:
    // someone is telling me that he may be my new predecessor
    XBT_DEBUG("Receiving a 'Notify' request from %s", message->issuer_host_name.c_str());
    notify(message->request_id);
    delete message;
    break;

  case PREDECESSOR_LEAVING:
    // my predecessor is about to quit
    XBT_DEBUG("Receiving a 'Predecessor Leaving' message from %s", message->issuer_host_name.c_str());
    // modify my predecessor
    setPredecessor(message->request_id);
    delete message;
    /*TODO :
      >> notify my new predecessor
      >> send a notify_predecessors !!
     */
    break;

  case SUCCESSOR_LEAVING:
    // my successor is about to quit
    XBT_DEBUG("Receiving a 'Successor Leaving' message from %s", message->issuer_host_name.c_str());
    // modify my successor FIXME : this should be implicit ?
    setFinger(0, message->request_id);
    delete message;
    /* TODO
       >> notify my new successor
       >> update my table & predecessors table */
    break;

  case PREDECESSOR_ALIVE:
    XBT_DEBUG("Receiving a 'Predecessor Alive' request from %s", message->issuer_host_name.c_str());
    message->type = PREDECESSOR_ALIVE_ANSWER;
    XBT_DEBUG("Sending back a 'Predecessor Alive Answer' to %s (mailbox %s)", message->issuer_host_name.c_str(),
              message->answer_to->get_cname());
    message->answer_to->put_init(message, 10)->detach(ChordMessage::destroy);
    break;

  default:
    XBT_DEBUG("Ignoring unexpected message: %d from %s", message->type, message->issuer_host_name.c_str());
    delete message;
  }
}
