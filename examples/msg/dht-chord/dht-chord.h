/* Copyright (c) 2016. The SimGrid Team.
* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef CHORD_CHORD_H_
#define CHORD_CHORD_H_
#include "simgrid/msg.h"
#include "simgrid/modelchecker.h"
#include <xbt/RngStream.h>
#include "src/mc/mc_replay.h" // FIXME: this is an internal header

#define COMM_SIZE 10
#define COMP_SIZE 0
#define MAILBOX_NAME_SIZE 10

/* Finger element. */
typedef struct s_finger {
  int id;
  char mailbox[MAILBOX_NAME_SIZE]; // string representation of the id
} s_finger_t;

/* Node data. */

typedef struct s_node {
  int id;                                 // my id
  char mailbox[MAILBOX_NAME_SIZE];        // my mailbox name (string representation of the id)
  s_finger_t *fingers;                    // finger table, of size nb_bits (fingers[0] is my successor)
  int pred_id;                            // predecessor id
  char pred_mailbox[MAILBOX_NAME_SIZE];   // predecessor's mailbox name
  int next_finger_to_fix;                 // index of the next finger to fix in fix_fingers()
  msg_comm_t comm_receive;                // current communication to receive
  double last_change_date;                // last time I changed a finger or my predecessor
  RngStream stream;                       //RngStream for
} s_node_t;
typedef s_node_t *node_t;

/* Types of tasks exchanged between nodes. */
typedef enum {
  TASK_FIND_SUCCESSOR,
  TASK_FIND_SUCCESSOR_ANSWER,
  TASK_GET_PREDECESSOR,
  TASK_GET_PREDECESSOR_ANSWER,
  TASK_NOTIFY,
  TASK_SUCCESSOR_LEAVING,
  TASK_PREDECESSOR_LEAVING,
  TASK_PREDECESSOR_ALIVE,
  TASK_PREDECESSOR_ALIVE_ANSWER
} e_task_type_t;

/* Data attached with the tasks sent and received */
typedef struct s_task_data {
  e_task_type_t type;                     // type of task
  int request_id;                         // id paramater (used by some types of tasks)
  int request_finger;                     // finger parameter (used by some types of tasks)
  int answer_id;                          // answer (used by some types of tasks)
  char answer_to[MAILBOX_NAME_SIZE];      // mailbox to send an answer to (if any)
  const char* issuer_host_name;           // used for logging
} s_task_data_t;
typedef s_task_data_t *task_data_t;

void create(node_t node);
int join(node_t node, int known_id);
void leave(node_t node);
int find_successor(node_t node, int id);
int remote_find_successor(node_t node, int ask_to_id, int id);
int remote_get_predecessor(node_t node, int ask_to_id);
int closest_preceding_node(node_t node, int id);
void stabilize(node_t node);
void notify(node_t node, int predecessor_candidate_id);
void remote_notify(node_t node, int notify_to, int predecessor_candidate_id);
void fix_fingers(node_t node);
void check_predecessor(node_t node);
void random_lookup(node_t node);
void quit_notify(node_t node);

#endif
