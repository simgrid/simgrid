/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "node.h"
#include "routing_table.h"
#include "simgrid/comm.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(dht_kademlia_node, "Messages specific for this example");

/** @brief Initialization of a node
 * @param node_id the id of the node
 * @return the node created
 */
node_t node_init(unsigned int node_id)
{
  node_t node             = xbt_new(s_node_t, 1);
  node->id                = node_id;
  node->table             = routing_table_init(node_id);
  node->mailbox           = get_node_mailbox(node_id);
  node->find_node_failed  = 0;
  node->find_node_success = 0;

  node->received_msg = NULL;
  node->receive_comm = NULL;

  return node;
}

/* @brief Node destructor  */
void node_free(node_t node)
{
  routing_table_free(node->table);
  xbt_free(node);
}

/**
  * Try to asynchronously get a new message from given mailbox. Return null if none available.
  */
kademlia_message_t receive(node_t node, sg_mailbox_t mailbox)
{
  if (node->receive_comm == NULL)
    node->receive_comm = sg_mailbox_get_async(mailbox, &node->received_msg);
  if (!sg_comm_test(node->receive_comm))
    return NULL;
  node->receive_comm = NULL;
  return node->received_msg;
}

/**
 * @brief Tries to join the network
 * @param node node data
 * @param id_known id of the node I know in the network.
 */
unsigned int join(node_t node, unsigned int id_known)
{
  unsigned int i;
  unsigned int got_answer = 0;

  sg_mailbox_t mailbox = get_node_mailbox(node->id);

  /* Add the guy we know to our routing table and ourselves. */
  routing_table_update(node, node->id);
  routing_table_update(node, id_known);

  /* First step: Send a "FIND_NODE" request to the node we know */
  send_find_node(node, id_known, node->id);
  do {
    const kademlia_message_t msg = receive(node, mailbox);
    if (msg) {
      XBT_DEBUG("Received an answer from the node I know.");
      got_answer = 1;
      // retrieve the node list and ping them.
      const s_answer_t* node_list  = msg->answer;
      if (node_list != NULL) {
        node_contact_t contact;
        xbt_dynar_foreach (node_list->nodes, i, contact)
          routing_table_update(node, contact->id);
        node->received_msg = NULL;
      } else {
        handle_find_node(node, msg);
      }
      answer_free(msg->answer);
      free(msg);
    } else {
      sg_actor_sleep_for(1);
    }
  } while (got_answer == 0);

  /* Second step: Send a FIND_NODE to a random node in buckets */
  unsigned int bucket_id = routing_table_find_bucket(node->table, id_known)->id;
  xbt_assert(bucket_id <= IDENTIFIER_SIZE);
  for (i = 0; ((bucket_id > i) || (bucket_id + i) <= IDENTIFIER_SIZE) && i < JOIN_BUCKETS_QUERIES; i++) {
    if (bucket_id > i) {
      unsigned int id_in_bucket = get_id_in_prefix(node->id, bucket_id - i);
      find_node(node, id_in_bucket, 0);
    }
    if (bucket_id + i <= IDENTIFIER_SIZE) {
      unsigned int id_in_bucket = get_id_in_prefix(node->id, bucket_id + i);
      find_node(node, id_in_bucket, 0);
    }
  }
  return got_answer;
}

/** @brief Send a "FIND_NODE" to a node
 * @param node sender node data
 * @param id node we are querying
 * @param destination node we are trying to find.
 */
void send_find_node(const_node_t node, unsigned int id, unsigned int destination)
{
  /* Gets the mailbox to send to */
  sg_mailbox_t mailbox = get_node_mailbox(id);
  /* Build the message */
  kademlia_message_t msg = new_message(node->id, destination, NULL, node->mailbox, sg_host_self_get_name());
  sg_comm_t comm         = sg_mailbox_put_init(mailbox, msg, COMM_SIZE);
  sg_comm_detach(comm, free_message);
  XBT_VERB("Asking %u for its closest nodes", id);
}

/**
 * Sends to the best "KADEMLIA_ALPHA" nodes in the "node_list" array a "FIND_NODE" request, to ask them for their best
 * nodes
 */
unsigned int send_find_node_to_best(const_node_t node, const_answer_t node_list)
{
  unsigned int i           = 0;
  unsigned int j           = 0;
  unsigned int destination = node_list->destination_id;
  while (j < KADEMLIA_ALPHA && i < node_list->size) {
    /* We need to have at most "KADEMLIA_ALPHA" requests each time, according to the protocol */
    /* Gets the node we want to send the query to */
    const s_node_contact_t* node_to_query = xbt_dynar_get_as(node_list->nodes, i, node_contact_t);
    if (node_to_query->id != node->id) { /* No need to query ourselves */
      send_find_node(node, node_to_query->id, destination);
      j++;
    }
    i++;
  }
  return i;
}

/** @brief Updates/Puts the node id unsigned into our routing table
 * @param node Our node data
 * @param id The id of the node we need to add unsigned into our routing table
 */
void routing_table_update(const_node_t node, unsigned int id)
{
  const_routing_table_t table = node->table;
  // retrieval of the bucket in which the should be
  const_bucket_t bucket = routing_table_find_bucket(table, id);

  // check if the id is already in the bucket.
  unsigned int id_pos = bucket_find_id(bucket, id);

  if (id_pos == -1) {
    /* We check if the bucket is full or not. If it is, we evict an old element */
    if (xbt_dynar_length(bucket->nodes) >= BUCKET_SIZE)
      xbt_dynar_pop(bucket->nodes, NULL);

    xbt_dynar_unshift(bucket->nodes, &id);
    XBT_VERB("I'm adding to my routing table %08x", id);
  } else {
    // We push to the front of the dynar the element.
    unsigned int element = xbt_dynar_get_as(bucket->nodes, id_pos, unsigned int);
    xbt_dynar_remove_at(bucket->nodes, id_pos, NULL);
    xbt_dynar_unshift(bucket->nodes, &element);
    XBT_VERB("I'm updating %08x", element);
  }
}

/** @brief Finds the closest nodes to the node given.
 * @param node : our node
 * @param destination_id : the id of the guy we are trying to find
 */
answer_t find_closest(const_node_t node, unsigned int destination_id)
{
  answer_t answer = answer_init(destination_id);
  /* We find the corresponding bucket for the id */
  const_bucket_t bucket = routing_table_find_bucket(node->table, destination_id);
  int bucket_id         = bucket->id;
  xbt_assert((bucket_id <= IDENTIFIER_SIZE), "Bucket found has a wrong identifier");
  /* So, we copy the contents of the bucket unsigned into our result dynar */
  answer_add_bucket(bucket, answer);

  /* However, if we don't have enough elements in our bucket, we NEED to include at least "BUCKET_SIZE" elements
   * (if, of course, we know at least "BUCKET_SIZE" elements. So we're going to look unsigned into the other buckets.
   */
  for (int i = 1; answer->size < BUCKET_SIZE && ((bucket_id - i > 0) || (bucket_id + i < IDENTIFIER_SIZE)); i++) {
    /* We check the previous buckets */
    if (bucket_id - i >= 0) {
      const_bucket_t bucket_p = &node->table->buckets[bucket_id - i];
      answer_add_bucket(bucket_p, answer);
    }
    /* We check the next buckets */
    if (bucket_id + i <= IDENTIFIER_SIZE) {
      const_bucket_t bucket_n = &node->table->buckets[bucket_id + i];
      answer_add_bucket(bucket_n, answer);
    }
  }
  /* We sort the array by XOR distance */
  answer_sort(answer);
  /* We trim the array to have only BUCKET_SIZE or less elements */
  answer_trim(answer);

  return answer;
}

unsigned int find_node(node_t node, unsigned int id_to_find, unsigned int count_in_stats)
{
  unsigned int queries;
  unsigned int answers;
  unsigned int destination_found = 0;
  unsigned int nodes_added       = 0;
  double global_timeout          = simgrid_get_clock() + FIND_NODE_GLOBAL_TIMEOUT;
  unsigned int steps             = 0;

  /* First we build a list of who we already know */
  answer_t node_list = find_closest(node, id_to_find);
  xbt_assert((node_list != NULL), "node_list incorrect");

  XBT_DEBUG("Doing a FIND_NODE on %08x", id_to_find);

  /* Ask the nodes on our list if they   have information about the node we are trying to find */
  sg_mailbox_t mailbox = get_node_mailbox(node->id);
  do {
    answers        = 0;
    queries        = send_find_node_to_best(node, node_list);
    nodes_added    = 0;
    double timeout = simgrid_get_clock() + FIND_NODE_TIMEOUT;
    steps++;
    double time_beginreceive = simgrid_get_clock();

    do {
      const kademlia_message_t msg = receive(node, mailbox);
      if (msg) {
        // Figure out if we received an answer or something else
        // Check if what we have received is what we are looking for.
        if (msg->answer != NULL && msg->answer->destination_id == id_to_find) {
          // Handle the answer
          routing_table_update(node, msg->sender_id);
          node_contact_t contact;
          unsigned int i;
          xbt_dynar_foreach (node_list->nodes, i, contact)
            routing_table_update(node, contact->id);

          answers++;

          nodes_added = answer_merge(node_list, msg->answer);
          XBT_DEBUG("Received an answer from %s (%s) with %lu nodes on it", sg_mailbox_get_name(msg->answer_to),
                    msg->issuer_host_name, xbt_dynar_length(msg->answer->nodes));
        } else {
          if (msg->answer != NULL) {
            routing_table_update(node, msg->sender_id);
            XBT_DEBUG("Received a wrong answer for a FIND_NODE");
          } else {
            handle_find_node(node, msg);
          }
          // Update the timeout if we didn't have our answer
          timeout += simgrid_get_clock() - time_beginreceive;
          time_beginreceive = simgrid_get_clock();
        }
        answer_free(msg->answer);
        free(msg);
      } else {
        sg_actor_sleep_for(1);
      }
    } while (simgrid_get_clock() < timeout && answers < queries);
    destination_found = answer_destination_found(node_list);
  } while (!destination_found && (nodes_added > 0 || answers == 0) && simgrid_get_clock() < global_timeout &&
           steps < MAX_STEPS);
  if (destination_found) {
    if (count_in_stats)
      node->find_node_success++;
    if (queries > 4)
      XBT_VERB("FIND_NODE on %08x success in %u steps", id_to_find, steps);
    routing_table_update(node, id_to_find);
  } else {
    if (count_in_stats) {
      node->find_node_failed++;
      XBT_VERB("%08x not found in %u steps", id_to_find, steps);
    }
  }
  answer_free(node_list);
  return destination_found;
}

/** @brief Does a pseudo-random lookup for someone in the system
 * @param node caller node data
 */
void random_lookup(node_t node)
{
  unsigned int id_to_look = RANDOM_LOOKUP_NODE; // Totally random.
  /* TODO: Use some pseudorandom generator. */
  XBT_DEBUG("I'm doing a random lookup");
  find_node(node, id_to_look, 1);
}

/** @brief Handles the answer to an incoming "find_node" message */
void handle_find_node(const_node_t node, const_kademlia_message_t msg)
{
  routing_table_update(node, msg->sender_id);
  XBT_VERB("Received a FIND_NODE from %s (%s), he's trying to find %08x", sg_mailbox_get_name(msg->answer_to),
           msg->issuer_host_name, msg->destination_id);
  // Building the msg to send
  kademlia_message_t answer = new_message(node->id, msg->destination_id, find_closest(node, msg->destination_id),
                                          node->mailbox, sg_host_self_get_name());
  // Sending the msg
  sg_comm_t comm = sg_mailbox_put_init(msg->answer_to, answer, COMM_SIZE);
  sg_comm_detach(comm, &free_message);
}

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

/** @brief Gets the mailbox name of a host given its identifier */
sg_mailbox_t get_node_mailbox(unsigned int id)
{
  char mailbox_name[MAILBOX_NAME_SIZE];
  snprintf(mailbox_name, MAILBOX_NAME_SIZE - 1, "%u", id);
  return sg_mailbox_by_name(mailbox_name);
}

/** Constructor, build a new contact information. */
node_contact_t node_contact_new(unsigned int id, unsigned int distance)
{
  node_contact_t contact = xbt_new(s_node_contact_t, 1);

  contact->id       = id;
  contact->distance = distance;

  return contact;
}

/** Builds a contact information from a contact information */
node_contact_t node_contact_copy(const_node_contact_t node_contact)
{
  node_contact_t contact = xbt_new(s_node_contact_t, 1);

  contact->id       = node_contact->id;
  contact->distance = node_contact->distance;

  return contact;
}

/** Destructor */
void node_contact_free(node_contact_t contact)
{
  xbt_free(contact);
}
