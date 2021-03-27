/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "node.hpp"
#include "routing_table.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(kademlia_node, "Messages specific for this example");

namespace kademlia {
static void destroy(void* message)
{
  const auto* msg = static_cast<Message*>(message);
  delete msg;
}

/**
  * Try to asynchronously get a new message from given mailbox. Return null if none available.
  */
Message* Node::receive(simgrid::s4u::Mailbox* mailbox)
{
  if (receive_comm == nullptr)
    receive_comm = mailbox->get_async<kademlia::Message>(&received_msg);
  if (not receive_comm->test())
    return nullptr;
  receive_comm = nullptr;
  return received_msg;
}

/**
  * @brief Tries to join the network
  * @param known_id id of the node I know in the network.
  */
bool Node::join(unsigned int known_id)
{
  bool got_answer = false;

  /* Add the guy we know to our routing table and ourselves. */
  routingTableUpdate(id_);
  routingTableUpdate(known_id);

  /* First step: Send a "FIND_NODE" request to the node we know */
  sendFindNode(known_id, id_);

  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(id_));
  do {
    if (Message* msg = receive(mailbox)) {
      XBT_DEBUG("Received an answer from the node I know.");
      got_answer = true;
      // retrieve the node list and ping them.
      const Answer* node_list = msg->answer_.get();
      if (node_list) {
        for (auto const& contact : node_list->getNodes())
          routingTableUpdate(contact.first);
      } else {
        handleFindNode(msg);
      }
      delete msg;
    } else
      simgrid::s4u::this_actor::sleep_for(1);
  } while (not got_answer);

  /* Second step: Send a FIND_NODE to a random node in buckets */
  unsigned int bucket_id = table.findBucket(known_id)->getId();
  xbt_assert(bucket_id <= IDENTIFIER_SIZE);
  for (unsigned int i = 0; ((bucket_id > i) || (bucket_id + i) <= IDENTIFIER_SIZE) && i < JOIN_BUCKETS_QUERIES; i++) {
    if (bucket_id > i) {
      unsigned int id_in_bucket = get_id_in_prefix(id_, bucket_id - i);
      findNode(id_in_bucket, false);
    }
    if (bucket_id + i <= IDENTIFIER_SIZE) {
      unsigned int id_in_bucket = get_id_in_prefix(id_, bucket_id + i);
      findNode(id_in_bucket, false);
    }
  }
  return got_answer;
}

/** @brief Send a "FIND_NODE" to a node
  * @param id node we are querying
  * @param destination node we are trying to find.
  */
void Node::sendFindNode(unsigned int id, unsigned int destination) const
{
  /* Gets the mailbox to send to */
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(id));
  /* Build the task */

  auto* msg = new Message(id_, destination, simgrid::s4u::Mailbox::by_name(std::to_string(id_)),
                          simgrid::s4u::Host::current()->get_cname());

  /* Send the task */
  mailbox->put_init(msg, 1)->detach(kademlia::destroy);
  XBT_VERB("Asking %u for its closest nodes", id);
}

/**
  * Sends to the best "KADEMLIA_ALPHA" nodes in the "node_list" array a "FIND_NODE" request, to ask them for their best
  * nodes
  */
unsigned int Node::sendFindNodeToBest(const Answer* node_list) const
{
  unsigned int i           = 0;
  unsigned int j           = 0;
  unsigned int destination = node_list->getDestinationId();
  for (auto const& node_to_query : node_list->getNodes()) {
    /* We need to have at most "KADEMLIA_ALPHA" requests each time, according to the protocol */
    /* Gets the node we want to send the query to */
    if (node_to_query.first != id_) { /* No need to query ourselves */
      sendFindNode(node_to_query.first, destination);
      j++;
    }
    i++;
    if (j == KADEMLIA_ALPHA)
      break;
  }
  return i;
}

/** @brief Updates/Puts the node id unsigned into our routing table
  * @param id The id of the node we need to add unsigned into our routing table
  */
void Node::routingTableUpdate(unsigned int id)
{
  // retrieval of the bucket in which the should be
  Bucket* bucket = table.findBucket(id);

  // check if the id is already in the bucket.
  auto id_pos = std::find(bucket->nodes_.begin(), bucket->nodes_.end(), id);

  if (id_pos == bucket->nodes_.end()) {
    /* We check if the bucket is full or not. If it is, we evict an old element */
    if (bucket->nodes_.size() >= BUCKET_SIZE) {
      bucket->nodes_.pop_back();
    }
    bucket->nodes_.push_front(id);
    XBT_VERB("I'm adding to my routing table %08x", id);
  } else {
    // We push the element to the front
    bucket->nodes_.erase(id_pos);
    bucket->nodes_.push_front(id);
    XBT_VERB("I'm updating %08x", id);
  }
}

/** @brief Finds the closest nodes to the node given.
  * @param node : our node
  * @param destination_id : the id of the guy we are trying to find
  */
std::unique_ptr<Answer> Node::findClosest(unsigned int destination_id)
{
  auto answer = std::make_unique<Answer>(destination_id);
  /* We find the corresponding bucket for the id */
  const Bucket* bucket = table.findBucket(destination_id);
  int bucket_id  = bucket->getId();
  xbt_assert((bucket_id <= IDENTIFIER_SIZE), "Bucket found has a wrong identifier");
  /* So, we copy the contents of the bucket unsigned into our answer */
  answer->addBucket(bucket);

  /* However, if we don't have enough elements in our bucket, we NEED to include at least "BUCKET_SIZE" elements
   * (if, of course, we know at least "BUCKET_SIZE" elements. So we're going to look unsigned into the other buckets.
   */
  for (int i = 1; answer->getSize() < BUCKET_SIZE && ((bucket_id - i > 0) || (bucket_id + i < IDENTIFIER_SIZE)); i++) {
    /* We check the previous buckets */
    if (bucket_id - i >= 0) {
      const Bucket* bucket_p = &table.getBucketAt(bucket_id - i);
      answer->addBucket(bucket_p);
    }
    /* We check the next buckets */
    if (bucket_id + i <= IDENTIFIER_SIZE) {
      const Bucket* bucket_n = &table.getBucketAt(bucket_id + i);
      answer->addBucket(bucket_n);
    }
  }
  /* We trim the array to have only BUCKET_SIZE or less elements */
  answer->trim();

  return answer;
}

/** @brief Send a request to find a node in the node routing table.
  * @param id_to_find the id of the node we are trying to find
  */
bool Node::findNode(unsigned int id_to_find, bool count_in_stats)
{
  unsigned int queries;
  unsigned int answers;
  bool destination_found   = false;
  unsigned int nodes_added = 0;
  double global_timeout    = simgrid::s4u::Engine::get_clock() + FIND_NODE_GLOBAL_TIMEOUT;
  unsigned int steps       = 0;

  /* First we build a list of who we already know */
  std::unique_ptr<Answer> node_list = findClosest(id_to_find);
  xbt_assert((node_list != nullptr), "node_list incorrect");
  XBT_DEBUG("Doing a FIND_NODE on %08x", id_to_find);

  /* Ask the nodes on our list if they have information about the node we are trying to find */
  do {
    answers        = 0;
    queries        = sendFindNodeToBest(node_list.get());
    nodes_added    = 0;
    double timeout = simgrid::s4u::Engine::get_clock() + FIND_NODE_TIMEOUT;
    steps++;
    double time_beginreceive = simgrid::s4u::Engine::get_clock();

    simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(id_));
    do {
      if (Message* msg = receive(mailbox)) {
        // Check if what we have received is what we are looking for.
        if (msg->answer_ && msg->answer_->getDestinationId() == id_to_find) {
          routingTableUpdate(msg->sender_id_);
          // Handle the answer
          for (auto const& contact : node_list->getNodes())
            routingTableUpdate(contact.first);
          answers++;

          nodes_added = node_list->merge(msg->answer_.get());
          XBT_DEBUG("Received an answer from %s (%s) with %zu nodes on it", msg->answer_to_->get_cname(),
                    msg->issuer_host_name_.c_str(), msg->answer_->getSize());
        } else {
          if (msg->answer_) {
            routingTableUpdate(msg->sender_id_);
            XBT_DEBUG("Received a wrong answer for a FIND_NODE");
          } else {
            handleFindNode(msg);
          }
          // Update the timeout if we didn't have our answer
          timeout += simgrid::s4u::Engine::get_clock() - time_beginreceive;
          time_beginreceive = simgrid::s4u::Engine::get_clock();
        }
        delete msg;
      } else {
        simgrid::s4u::this_actor::sleep_for(1);
      }
    } while (simgrid::s4u::Engine::get_clock() < timeout && answers < queries);
    destination_found = node_list->destinationFound();
  } while (not destination_found && (nodes_added > 0 || answers == 0) &&
           simgrid::s4u::Engine::get_clock() < global_timeout && steps < MAX_STEPS);

  if (destination_found) {
    if (count_in_stats)
      find_node_success++;
    if (queries > 4)
      XBT_VERB("FIND_NODE on %08x success in %u steps", id_to_find, steps);
    routingTableUpdate(id_to_find);
  } else {
    if (count_in_stats) {
      find_node_failed++;
      XBT_VERB("%08x not found in %u steps", id_to_find, steps);
    }
  }
  return destination_found;
}

/** @brief Does a pseudo-random lookup for someone in the system
  * @param node caller node data
  */
void Node::randomLookup()
{
  unsigned int id_to_look = RANDOM_LOOKUP_NODE; // Totally random.
  /* TODO: Use some pseudo-random generator. */
  XBT_DEBUG("I'm doing a random lookup");
  findNode(id_to_look, true);
}

/** @brief Handles the answer to an incoming "find_node" task */
void Node::handleFindNode(const Message* msg)
{
  routingTableUpdate(msg->sender_id_);
  XBT_VERB("Received a FIND_NODE from %s (%s), he's trying to find %08x", msg->answer_to_->get_cname(),
           msg->issuer_host_name_.c_str(), msg->destination_id_);
  // Building the answer to the request
  auto* answer =
      new Message(id_, msg->destination_id_, findClosest(msg->destination_id_),
                  simgrid::s4u::Mailbox::by_name(std::to_string(id_)), simgrid::s4u::Host::current()->get_cname());
  // Sending the answer
  msg->answer_to_->put_init(answer, 1)->detach(kademlia::destroy);
}

void Node::displaySuccessRate() const
{
  XBT_INFO("%u/%u FIND_NODE have succeeded", find_node_success, find_node_success + find_node_failed);
}
} // namespace kademlia
/**@brief Returns an identifier which is in a specific bucket of a routing table
 * @param id id of the routing table owner
 * @param prefix id of the bucket where we want that identifier to be
 */
unsigned int get_id_in_prefix(unsigned int id, unsigned int prefix)
{
  if (prefix == 0) {
    return 0;
  } else {
    return (1U << (prefix - 1)) ^ id;
  }
}

/** @brief Returns the prefix of an identifier.
  * The prefix is the id of the bucket in which the remote identifier xor our identifier should be stored.
  * @param id : big unsigned int id to test
  * @param nb_bits : key size
  */
unsigned int get_node_prefix(unsigned int id, unsigned int nb_bits)
{
  unsigned int size = sizeof(unsigned int) * 8;
  for (unsigned int j = 0; j < size; j++) {
    if (((id >> (size - 1 - j)) & 0x1) != 0) {
      return nb_bits - j;
    }
  }
  return 0;
}
