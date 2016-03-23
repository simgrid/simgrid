/* Copyright (c) 2010, 2012-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "node.h"
#include "routing_table.h"
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kademlia_node, "Messages specific for this msg example");

/** @brief Initialization of a node
  * @param node_id the id of the node
  * @return the node created
  */
node_t node_init(unsigned int node_id)
{
  node_t node = xbt_new(s_node_t, 1);

  node->id = node_id;
  node->table = routing_table_init(node_id);
  sprintf(node->mailbox, "%0*x", MAILBOX_NAME_SIZE, node_id);
  node->find_node_failed = 0;
  node->find_node_success = 0;

  node->task_received = NULL;
  node->receive_comm = NULL;

  return node;
}

/* @brief Node destructor  */
void node_free(node_t node)
{
  routing_table_free(node->table);
  xbt_free(node);
}

/** @brief Updates/Puts the node id unsigned into our routing table
  * @param node Our node data
  * @param id The id of the node we need to add unsigned into our routing table
  */
void node_routing_table_update(node_t node, unsigned int id)
{
  routing_table_t table = node->table;
  //retrieval of the bucket in which the should be
  bucket_t bucket = routing_table_find_bucket(table, id);

  //check if the id is already in the bucket.
  unsigned int id_pos = bucket_find_id(bucket, id);

  if (id_pos == -1) {
    /* We check if the bucket is full or not. If it is, we evict old offline elements */
    if (xbt_dynar_length(bucket->nodes) < bucket_size) {
      //TODO: this is not really very efficient. Maybe we should use something else than dynars ?
      xbt_dynar_unshift(bucket->nodes, &id);
      XBT_VERB("I'm adding to my routing table %08x", id);
    } else {
      /* TODO: we need to evict the old elements: that's why this function is in "node" instead of "routing table".
       * This is not implemented yet. */
    }
  } else {
    //We push to the front of the dynar the element.
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
answer_t node_find_closest(node_t node, unsigned int destination_id)
{
  int i;
  answer_t answer = answer_init(destination_id);
  /* We find the corresponding bucket for the id */
  bucket_t bucket = routing_table_find_bucket(node->table, destination_id);
  int bucket_id = bucket->id;
  xbt_assert((bucket_id <= identifier_size), "Bucket found has a wrong identifier");
  /* So, we copy the contents of the bucket unsigned into our result dynar */
  answer_add_bucket(bucket, answer);

  /* However, if we don't have enough elements in our bucket, we NEED to include at least "bucket_size" elements
   * (if, of course, we know at least "bucket_size" elements. So we're going to look unsigned into the other buckets.
   */
  for (i = 1; answer->size < bucket_size && ((bucket_id - i > 0) || (bucket_id + i < identifier_size)); i++) {
    /* We check the previous buckets */
    if (bucket_id - i >= 0) {
      bucket_t bucket_p = &node->table->buckets[bucket_id - i];
      answer_add_bucket(bucket_p, answer);
    }
    /* We check the next buckets */
    if (bucket_id + i <= identifier_size) {
      bucket_t bucket_n = &node->table->buckets[bucket_id + i];
      answer_add_bucket(bucket_n, answer);
    }
  }
  /* We sort the array by XOR distance */
  answer_sort(answer);
  /* We trim the array to have only bucket_size or less elements */
  answer_trim(answer);

  return answer;
}

/**@brief Returns an identifier which is in a specific bucket of a routing table
 * @param id id of the routing table owner
 * @param prefix id of the bucket where we want that identifier to be
 */
unsigned int get_id_in_prefix(unsigned int id, unsigned int prefix)
{
  if (prefix == 0) {
    return 0;
  }
  unsigned int n = 1 << (prefix - 1);
  return n ^ id;
}

/** @brief Returns the prefix of an identifier.
  * The prefix is the id of the bucket in which the remote identifier xor our identifier should be stored.
  * @param id : bigunsigned int id to test
  * @param nb_bits : key size
  */
unsigned int get_node_prefix(unsigned int id, unsigned int nb_bits)
{
  unsigned int j, size = sizeof(unsigned int) * 8;
  for (j = 0; j < size; j++) {
    if (((id >> (size - 1 - j)) & 0x1) != 0) {
      return nb_bits - (j);
    }
  }
  return 0;
}

/** @brief Gets the mailbox name of a host given its identifier
  */
void get_node_mailbox(unsigned int id, char *mailbox)
{
  sprintf(mailbox, "%0*x", MAILBOX_NAME_SIZE, id);
}

/** Constructor, build a new contact information. */
node_contact_t node_contact_new(unsigned int id, unsigned int distance)
{
  node_contact_t contact = xbt_new(s_node_contact_t, 1);

  contact->id = id;
  contact->distance = distance;

  return contact;
}

/** Builds a contact information from a contact information */
node_contact_t node_contact_copy(node_contact_t node_contact)
{
  node_contact_t contact = xbt_new(s_node_contact_t, 1);

  contact->id = node_contact->id;
  contact->distance = node_contact->distance;

  return contact;
}

/** Destructor */
void node_contact_free(node_contact_t contact)
{
  xbt_free(contact);
}
