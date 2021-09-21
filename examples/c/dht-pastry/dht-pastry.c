/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/mailbox.h"

#include "xbt/dynar.h"
#include "xbt/ex.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/sysdep.h"

#include <stdio.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(pastry, "Messages specific for this example");

#define COMM_SIZE 10
#define COMP_SIZE 0
#define MAILBOX_NAME_SIZE 10

#define DOMAIN_SIZE 4
#define LEVELS_COUNT 8 // sizeof(int)*8/DOMAIN_SIZE
#define LEVEL_SIZE 16  // 2^DOMAIN_SIZE
#define NEIGHBORHOOD_SIZE 6
#define NAMESPACE_SIZE 6
#define MAILBOX_NAME_SIZE 10

static int nb_bits             = 16;
static int timeout             = 50;
static int max_simulation_time = 1000;

typedef struct s_node {
  int id; // 128bits generated random(2^128 -1)
  int known_id;
  sg_mailbox_t mailbox;
  int namespace_set[NAMESPACE_SIZE];
  int neighborhood_set[NEIGHBORHOOD_SIZE];
  int routing_table[LEVELS_COUNT][LEVEL_SIZE];
  int ready;
  sg_comm_t comm_receive; // current communication to receive
  xbt_dynar_t pending_messages;
} s_node_t;
typedef s_node_t* node_t;
typedef const s_node_t* const_node_t;

typedef struct s_state {
  int id;
  int namespace_set[NAMESPACE_SIZE];
  int neighborhood_set[NEIGHBORHOOD_SIZE];
  int routing_table[LEVELS_COUNT][LEVEL_SIZE];
} s_state_t;
typedef s_state_t* state_t;

/** Types of tasks exchanged between nodes. */
typedef enum { JOIN, JOIN_REPLY, JOIN_LAST_REPLY, UPDATE } e_message_type_t;

typedef struct s_pastry_message {
  e_message_type_t type; // type of task
  int sender_id;         // id parameter (used by some types of tasks)
  // int request_finger;           // finger parameter (used by some types of tasks)
  int answer_id;          // answer (used by some types of tasks)
  sg_mailbox_t answer_to; // mailbox to send an answer to (if any)
  int steps;
  state_t state;
} s_pastry_message_t;
typedef s_pastry_message_t* pastry_message_t;
typedef const s_pastry_message_t* const_pastry_message_t;

/** Get the mailbox of a host given its pastry id. */
static sg_mailbox_t get_mailbox(int node_id)
{
  char mailbox_name[MAILBOX_NAME_SIZE];
  snprintf(mailbox_name, MAILBOX_NAME_SIZE - 1, "%d", node_id);
  return sg_mailbox_by_name(mailbox_name);
}

/** Get the specific level of a node id */
unsigned int domain_mask = 0;
static int domain(unsigned int a, unsigned int level)
{
  if (domain_mask == 0)
    domain_mask = (1U << DOMAIN_SIZE) - 1;
  unsigned int shift = (LEVELS_COUNT - level - 1) * DOMAIN_SIZE;
  return (a >> shift) & domain_mask;
}

/* Get the shared domains between the two givens ids */
static int shl(int a, int b)
{
  int l = 0;
  while (l < LEVELS_COUNT && domain(a, l) == domain(b, l))
    l++;
  return l;
}

/* Frees the memory used by a task and destroy it */
static void message_free(pastry_message_t message)
{
  if (message != NULL) {
    xbt_free(message->state);
    xbt_free(message);
  }
}

/* Get the closest id to the dest in the node namespace_set */
static int closest_in_namespace_set(const_node_t node, int dest)
{
  int res = -1;
  if ((node->namespace_set[NAMESPACE_SIZE - 1] <= dest) && (dest <= node->namespace_set[0])) {
    int best_dist = abs(node->id - dest);
    res           = node->id;
    for (int i = 0; i < NAMESPACE_SIZE; i++) {
      if (node->namespace_set[i] != -1) {
        int dist = abs(node->namespace_set[i] - dest);
        if (dist < best_dist) {
          best_dist = dist;
          res       = node->namespace_set[i];
        }
      }
    }
  }
  return res;
}

/* Find the next node to forward a message to */
static int routing_next(const_node_t node, int dest)
{
  int closest = closest_in_namespace_set(node, dest);
  if (closest != -1)
    return closest;

  int l   = shl(node->id, dest);
  int res = node->routing_table[l][domain(dest, l)];
  if (res != -1)
    return res;

  // rare case
  int dist = abs(node->id - dest);
  for (int i = l; i < LEVELS_COUNT; i++) {
    for (int j = 0; j < LEVEL_SIZE; j++) {
      res = node->routing_table[i][j];
      if (res != -1 && abs(res - dest) < dist)
        return res;
    }
  }

  for (int i = 0; i < NEIGHBORHOOD_SIZE; i++) {
    res = node->neighborhood_set[i];
    if (res != -1 && shl(res, dest) >= l && abs(res - dest) < dist)
      return res;
  }

  for (int i = 0; i < NAMESPACE_SIZE; i++) {
    res = node->namespace_set[i];
    if (res != -1 && shl(res, dest) >= l && abs(res - dest) < dist)
      return res;
  }

  return node->id;
}

/* Get the corresponding state of a node */
static state_t node_get_state(const_node_t node)
{
  state_t state = xbt_new0(s_state_t, 1);
  state->id     = node->id;
  for (int i = 0; i < NEIGHBORHOOD_SIZE; i++)
    state->neighborhood_set[i] = node->neighborhood_set[i];

  for (int i = 0; i < LEVELS_COUNT; i++)
    for (int j = 0; j < LEVEL_SIZE; j++)
      state->routing_table[i][j] = node->routing_table[i][j];

  for (int i = 0; i < NAMESPACE_SIZE; i++)
    state->namespace_set[i] = node->namespace_set[i];

  return state;
}

static void print_node_id(const_node_t node)
{
  XBT_INFO(" Id: %i '%08x' ", node->id, (unsigned)node->id);
}

/* Print the node namespace set */
static void print_node_namespace_set(const_node_t node)
{
  XBT_INFO(" Namespace:");
  for (int i = 0; i < NAMESPACE_SIZE; i++)
    XBT_INFO("  %08x", (unsigned)node->namespace_set[i]);
}

/** Handle a given task */
static void handle_message(node_t node, pastry_message_t message)
{
  XBT_DEBUG("Handling task %p", message);
  int i;
  int j;
  int min;
  int max;
  int next;
  sg_mailbox_t mailbox;
  sg_comm_t comm = NULL;
  sg_error_t err = SG_OK;
  pastry_message_t request;
  e_message_type_t type = message->type;

  // If the node is not ready keep the task for later
  if (node->ready != 0 && !(type == JOIN_LAST_REPLY || type == JOIN_REPLY)) {
    XBT_DEBUG("Task pending %u", type);
    xbt_dynar_push(node->pending_messages, &message);
    return;
  }

  switch (type) {
    /* Try to join the ring */
    case JOIN:
      next = routing_next(node, message->answer_id);
      XBT_DEBUG("Join request from %08x forwarding to %08x", (unsigned)message->answer_id, (unsigned)next);
      type = JOIN_LAST_REPLY;

      request            = xbt_new0(s_pastry_message_t, 1);
      request->answer_id = message->sender_id;
      request->steps     = message->steps + 1;

      // if next different from current node forward the join
      if (next != node->id) {
        mailbox            = get_mailbox(next);
        message->sender_id = node->id;
        message->steps++;
        comm = sg_mailbox_put_async(mailbox, message, COMM_SIZE);
        err  = sg_comm_wait_for(comm, timeout);
        if (err == SG_ERROR_TIMEOUT) {
          XBT_DEBUG("Timeout expired when forwarding join to next %d", next);
          xbt_free(request);
          break;
        }
        type = JOIN_REPLY;
      }

      // send back the current node state to the joining node
      request->type      = type;
      request->sender_id = node->id;
      request->answer_to = get_mailbox(node->id);
      request->state     = node_get_state(node);
      comm               = sg_mailbox_put_async(message->answer_to, request, COMM_SIZE);
      err                = sg_comm_wait_for(comm, timeout);
      if (err == SG_ERROR_TIMEOUT) {
        XBT_DEBUG("Timeout expired when sending back the current node state to the joining node to %d", node->id);
        message_free(request);
      }
      break;
    /* Join reply from all the node touched by the join  */
    case JOIN_LAST_REPLY:
      // if last node touched reply, copy its namespace set
      // TODO: it works only if the two nodes are side to side (is it really the case ?)
      j = (message->sender_id < node->id) ? -1 : 0;
      for (i = 0; i < NAMESPACE_SIZE / 2; i++) {
        node->namespace_set[i]                      = message->state->namespace_set[i - j];
        node->namespace_set[NAMESPACE_SIZE - 1 - i] = message->state->namespace_set[NAMESPACE_SIZE - 1 - i - j - 1];
      }
      node->namespace_set[NAMESPACE_SIZE / 2 + j] = message->sender_id;
      node->ready += message->steps + 1;
      /* no break */
    case JOIN_REPLY:
      XBT_DEBUG("Joining Reply");

      // if first node touched reply, copy its neighborhood set
      if (message->sender_id == node->known_id) {
        node->neighborhood_set[0] = message->sender_id;
        for (i = 1; i < NEIGHBORHOOD_SIZE; i++)
          node->neighborhood_set[i] = message->state->neighborhood_set[i - 1];
      }

      // copy the corresponding routing table levels
      min = (node->id == message->answer_id) ? 0 : shl(node->id, message->answer_id);
      max = shl(node->id, message->sender_id) + 1;
      for (i = min; i < max; i++) {
        int d = domain(node->id, i);
        for (j = 0; j < LEVEL_SIZE; j++)
          if (d != j)
            node->routing_table[i][j] = message->state->routing_table[i][j];
      }

      node->ready--;
      // if the node is ready, do all the pending tasks and send update to known nodes
      if (node->ready == 0) {
        XBT_DEBUG("Node %i is ready!!!", node->id);
        while (!xbt_dynar_is_empty(node->pending_messages)) {
          pastry_message_t m;
          xbt_dynar_shift(node->pending_messages, &m);
          handle_message(node, m);
        }

        for (i = 0; i < NAMESPACE_SIZE; i++) {
          j = node->namespace_set[i];
          if (j != -1) {
            XBT_DEBUG("Send update to %i", j);
            mailbox = get_mailbox(j);

            request            = xbt_new0(s_pastry_message_t, 1);
            request->answer_id = node->id;
            request->steps     = 0;
            request->type      = UPDATE;
            request->sender_id = node->id;
            request->answer_to = get_mailbox(node->id);
            request->state     = node_get_state(node);
            comm               = sg_mailbox_put_async(mailbox, request, COMM_SIZE);
            err                = sg_comm_wait_for(comm, timeout);
            if (err == SG_ERROR_TIMEOUT) {
              XBT_DEBUG("Timeout expired when sending update to %d", j);
              message_free(request);
              break;
            }
          }
        }
      }
      break;
    /* Received an update of state */
    case UPDATE:
      XBT_DEBUG("Task update %i !!!", node->id);

      /* Update namespace ses */
      XBT_INFO("Task update from %i !!!", message->sender_id);
      XBT_INFO("Node:");
      print_node_id(node);
      print_node_namespace_set(node);
      int curr_namespace_set[NAMESPACE_SIZE];
      int task_namespace_set[NAMESPACE_SIZE + 1];

      // Copy the current namespace and the task state namespace with state->id in the middle
      i = 0;
      for (; i < NAMESPACE_SIZE / 2; i++) {
        curr_namespace_set[i] = node->namespace_set[i];
        task_namespace_set[i] = message->state->namespace_set[i];
      }
      task_namespace_set[i] = message->state->id;
      for (; i < NAMESPACE_SIZE; i++) {
        curr_namespace_set[i]     = node->namespace_set[i];
        task_namespace_set[i + 1] = message->state->namespace_set[i];
      }

      // get the index of values before and after node->id in task_namespace
      min = -1;
      max = -1;
      for (i = 0; i <= NAMESPACE_SIZE; i++) {
        j = task_namespace_set[i];
        if (j != -1 && j < node->id)
          min = i;
        if (j != -1 && max == -1 && j > node->id)
          max = i;
      }

      // add lower elements
      j = NAMESPACE_SIZE / 2 - 1;
      for (i = NAMESPACE_SIZE / 2 - 1; i >= 0; i--) {
        if (min < 0 || curr_namespace_set[j] > task_namespace_set[min]) {
          node->namespace_set[i] = curr_namespace_set[j];
          j--;
        } else if (curr_namespace_set[j] == task_namespace_set[min]) {
          node->namespace_set[i] = curr_namespace_set[j];
          j--;
          min--;
        } else {
          node->namespace_set[i] = task_namespace_set[min];
          min--;
        }
      }

      // add greater elements
      j = NAMESPACE_SIZE / 2;
      for (i = NAMESPACE_SIZE / 2; i < NAMESPACE_SIZE; i++) {
        if (min < 0 || max >= NAMESPACE_SIZE) {
          node->namespace_set[i] = curr_namespace_set[j];
          j++;
        } else if (max >= 0) {
          if (curr_namespace_set[j] == -1 || curr_namespace_set[j] > task_namespace_set[max]) {
            node->namespace_set[i] = task_namespace_set[max];
            max++;
          } else if (curr_namespace_set[j] == task_namespace_set[max]) {
            node->namespace_set[i] = curr_namespace_set[j];
            j++;
            max++;
          } else {
            node->namespace_set[i] = curr_namespace_set[j];
            j++;
          }
        }
      }

      /* Update routing table */
      for (i = shl(node->id, message->state->id); i < LEVELS_COUNT; i++) {
        for (j = 0; j < LEVEL_SIZE; j++) {
          // FIXME: this is a no-op!
          if (node->routing_table[i][j] == -1 && message->state->routing_table[i][j] == -1)
            node->routing_table[i][j] = message->state->routing_table[i][j];
        }
      }
      break;
    default:
      THROW_IMPOSSIBLE;
  }
  message_free(message);
}

/* Join the ring */
static int join(const_node_t node)
{
  pastry_message_t request = xbt_new0(s_pastry_message_t, 1);
  request->type            = JOIN;
  request->sender_id       = node->id;
  request->answer_id       = node->id;
  request->steps           = 0;
  request->answer_to       = get_mailbox(node->id);

  sg_mailbox_t mailbox = get_mailbox(node->known_id);

  XBT_DEBUG("Trying to join Pastry ring... (with node %s)", sg_mailbox_get_name(mailbox));
  sg_comm_t comm = sg_mailbox_put_async(mailbox, request, COMM_SIZE);
  sg_error_t err = sg_comm_wait_for(comm, timeout);
  if (err == SG_ERROR_TIMEOUT) {
    XBT_DEBUG("Timeout expired when joining ring with node %d", node->known_id);
    message_free(request);
    return 0;
  }

  return 1;
}

/**
 * @brief Node Function
 * Arguments:
 * - my id
 * - the id of a guy I know in the system (except for the first node)
 * - the time to sleep before I join (except for the first node)
 * - the deadline time
 */
static void node(int argc, char* argv[])
{
  double init_time = simgrid_get_clock();
  void* received   = NULL;
  int join_success = 0;
  double deadline;
  xbt_assert(argc == 3 || argc == 5, "Wrong number of arguments for this node");
  s_node_t node         = {0};
  node.id               = (int)xbt_str_parse_int(argv[1], "Invalid ID");
  node.known_id         = -1;
  node.ready            = -1;
  node.pending_messages = xbt_dynar_new(sizeof(pastry_message_t), NULL);
  node.mailbox          = get_mailbox(node.id);

  XBT_DEBUG("New node with id %s (%08x)", sg_mailbox_get_name(node.mailbox), (unsigned)node.id);

  for (int i = 0; i < LEVELS_COUNT; i++) {
    int d = domain(node.id, i);
    for (int j = 0; j < LEVEL_SIZE; j++)
      node.routing_table[i][j] = (d == j) ? node.id : -1;
  }

  for (int i = 0; i < NEIGHBORHOOD_SIZE; i++)
    node.neighborhood_set[i] = -1;

  for (int i = 0; i < NAMESPACE_SIZE; i++)
    node.namespace_set[i] = -1;

  if (argc == 3) { // first ring
    XBT_DEBUG("Hey! Let's create the system.");
    deadline   = xbt_str_parse_double(argv[2], "Invalid deadline");
    node.ready = 0;
    XBT_DEBUG("Create a new Pastry ring...");
    join_success = 1;
  } else {
    node.known_id     = (int)xbt_str_parse_int(argv[2], "Invalid known ID");
    double sleep_time = xbt_str_parse_double(argv[3], "Invalid sleep time");
    deadline          = xbt_str_parse_double(argv[4], "Invalid deadline");

    // sleep before starting
    XBT_DEBUG("Let's sleep during %f", sleep_time);
    sg_actor_sleep_for(sleep_time);
    XBT_DEBUG("Hey! Let's join the system.");

    join_success = join(&node);
  }

  if (join_success) {
    XBT_DEBUG("Waiting ….");

    while (simgrid_get_clock() < init_time + deadline && simgrid_get_clock() < max_simulation_time) {
      if (node.comm_receive == NULL) {
        received          = NULL;
        node.comm_receive = sg_mailbox_get_async(node.mailbox, &received);
      }
      if (!sg_comm_test(node.comm_receive)) {
        sg_actor_sleep_for(5);
      } else {
        // the task was successfully received
        handle_message(&node, received);
        node.comm_receive = NULL;
      }
    }
    // Cleanup the receiving communication.
    if (node.comm_receive != NULL)
      sg_comm_unref(node.comm_receive);
  }
  xbt_dynar_free(&node.pending_messages);
}

/** @brief Main function. */
int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s [-nb_bits=n] [-timeout=t] platform_file deployment_file\n"
             "\tExample: %s ../platform.xml pastry10.xml\n",
             argv[0], argv[0]);

  char** options = &argv[1];
  while (!strncmp(options[0], "-", 1)) {
    int length = strlen("-nb_bits=");
    if (!strncmp(options[0], "-nb_bits=", length) && strlen(options[0]) > length) {
      nb_bits = (int)xbt_str_parse_int(options[0] + length, "Invalid nb_bits parameter");
      XBT_DEBUG("Set nb_bits to %d", nb_bits);
    } else {
      length = strlen("-timeout=");
      xbt_assert(strncmp(options[0], "-timeout=", length) == 0 && strlen(options[0]) > length,
                 "Invalid pastry option '%s'", options[0]);
      timeout = (int)xbt_str_parse_int(options[0] + length, "Invalid timeout parameter");
      XBT_DEBUG("Set timeout to %d", timeout);
    }
    options++;
  }

  simgrid_load_platform(options[0]);

  simgrid_register_function("node", node);
  simgrid_load_deployment(options[1]);

  simgrid_run();
  XBT_INFO("Simulated time: %g", simgrid_get_clock());

  return 0;
}
