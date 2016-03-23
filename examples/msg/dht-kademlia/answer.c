/* Copyright (c) 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "answer.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_kademlia_node);

/** Initialize a node answer object. */
answer_t answer_init(unsigned int destination_id)
{
  answer_t answer = xbt_new(s_answer_t, 1);
  answer->nodes = xbt_dynar_new(sizeof(node_contact_t), NULL);
  answer->size = 0;
  answer->destination_id = destination_id;

  return answer;
}

/** Destroys a node answer object. */
void answer_free(answer_t answer)
{
  unsigned int i;
  for (i = 0; i < answer->size; i++) {
    node_contact_free(*(void **) xbt_dynar_get_ptr(answer->nodes, i));
  }
  xbt_dynar_free(&answer->nodes);
  xbt_free(answer);
}

/** @brief Prints a answer_t, for debugging purposes */
void answer_print(answer_t answer)
{
  unsigned int cpt;
  node_contact_t contact;
  XBT_INFO("Searching %08x, size %d", answer->destination_id, answer->size);
  xbt_dynar_foreach(answer->nodes, cpt, contact) {
    XBT_INFO("Node %08x: %08x is at distance %d", cpt, contact->id, contact->distance);
  }
}

/** @brief Merge two answer_t together, only keeping the best nodes
  * @param destination the destination in which the nodes will be put
  * @param source the source of the nodes to add
  */
unsigned int answer_merge(answer_t destination, answer_t source)
{
  node_contact_t contact, contact_copy;
  unsigned int cpt;
  unsigned int nb_added = 0;
  /* TODO: Check if same destination */
  xbt_dynar_foreach(source->nodes, cpt, contact) {
    if (!answer_contains(destination, contact->id)) {
      contact_copy = node_contact_copy(contact);
      xbt_dynar_push(destination->nodes, &contact_copy);
      destination->size++;
      nb_added++;
    }
  }
  answer_sort(destination);
  answer_trim(destination);
  return nb_added;
}

/** Helper to sort answer_t objects */
static int _answer_sort_function(const void *e1, const void *e2)
{
  node_contact_t c1 = *(void **) e1;
  node_contact_t c2 = *(void **) e2;
  if (c1->distance == c2->distance)
    return 0;
  else
    if (c1->distance < c2->distance)
      return -1;
    else
      return 1;
}

/** @brief Sorts a answer_t, by node distance.
  * @param answer the answer to sort
  * @param destination_id the id of the guy we are trying to find
  */
void answer_sort(answer_t answer)
{
  xbt_dynar_sort(answer->nodes, &_answer_sort_function);
}

/** @brief Trims a answer_t, in order for it to have a size of less or equal to "bucket_size"
  * @param answer the answer_t to trim
  */
void answer_trim(answer_t answer)
{
  node_contact_t value;
  while (answer->size > bucket_size) {
    xbt_dynar_pop(answer->nodes, &value);
    answer->size--;
    node_contact_free(value);
  }
  xbt_assert(xbt_dynar_length(answer->nodes) == answer->size, "Wrong size for the answer");
}

/** @brief Adds the content of a bucket unsigned into a answer object.
  * @param bucket the bucket we have to had unsigned into
  * @param answer the answer object we're going  to put the data in
  * @param destination_id the id of the guy we are trying to find.
  */
void answer_add_bucket(bucket_t bucket, answer_t answer)
{
  unsigned int cpt;
  unsigned int id, distance;
  node_contact_t contact;
  xbt_assert((bucket != NULL), "Provided a NULL bucket");
  xbt_assert((bucket->nodes != NULL), "Provided a bucket which nodes are NULL");
  xbt_dynar_foreach(bucket->nodes, cpt, id) {
    distance = id ^ answer->destination_id;
    contact = node_contact_new(id, distance);
    xbt_dynar_push(answer->nodes, &contact);
    answer->size++;
  }
}

/** @brief Returns if the id supplied is in the answer.
  * @param id : id we're looking for
  */
unsigned int answer_contains(answer_t answer, unsigned int id)
{
  unsigned int i = 0;
  node_contact_t contact;
  xbt_dynar_foreach(answer->nodes, i, contact){
    if (id == contact->id) {
      return 1;
    }
  }
  return 0;
}

/** @brief Returns if the destination we are trying to find is found
  * @param answer the answer
  * @return if the destination is found.
  */
unsigned int answer_destination_found(answer_t answer)
{
  if (xbt_dynar_length(answer->nodes) < 1) {
    return 0;
  }
  node_contact_t contact_tail = xbt_dynar_get_as(answer->nodes, 0, node_contact_t);
  return contact_tail->distance == 0;
}
