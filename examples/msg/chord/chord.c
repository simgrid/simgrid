
/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_chord,
                             "Messages specific for this msg example");

#define NB_BITS 6
#define NB_KEYS 64

/**
 * Finger element.
 */
typedef struct finger {
  int id;
  char* mailbox;
} s_finger_t, *finger_t;

/**
 * Node data.
 */
typedef struct node {
  int id;                                 // my id
  char* mailbox;                          // my usual mailbox name
  s_finger_t fingers[NB_BITS];            // finger table (fingers[0] is my successor)
  int pred_id;                            // predecessor id
  char* pred_mailbox;                     // predecessor's mailbox name
  int next_finger_to_fix;                 // index of the next finger to fix in fix_fingers()
  xbt_dynar_t comms;                      // current communications pending
} s_node_t, *node_t;

/**
 * Task data
 */
typedef struct task_data {
  int request_id;
  int request_finger;
  int answer_id;
  const char* answer_to;
  const char* issuer_host_name; // used for logging
  int successor_id;				// used when quitting
  int pred_id;  				// used when quitting
  // FIXME: remove successor_id and pred_id, request_id is enough
} s_task_data_t, *task_data_t;

static int powers2[NB_BITS];

// utility functions
static void chord_initialize(void);
static int normalize(int id);
static int is_in_interval(int id, int start, int end);
static char* get_mailbox(int host_id);
static void print_finger_table(node_t node);
static void set_finger(node_t node, int finger_index, int id);
static void set_predecessor(node_t node, int predecessor_id);

// process functions
static int node(int argc, char *argv[]);

// Chord core
static void create(node_t node);
static void join(node_t node, int known_id);
static void leave(node_t node);
static int find_successor(node_t node, int id);
static int remote_find_successor(node_t node, int ask_to_id, int id);
static int remote_get_predecessor(node_t node, int ask_to_id);
static int closest_preceding_node(node_t node, int id);
static void stabilize(node_t node);
static void notify(node_t node, int predecessor_candidate_id);
static void remote_notify(node_t node, int notify_to, int predecessor_candidate_id);
static void fix_fingers(node_t node);
static void check_predecessor(node_t node);
static void quit_notify(node_t node, int to);

// not implemented yet
//static void remote_move_keys(node_t node, int take_from_id);

// deprecated
/*
static void bootstrap(node_t node, int known_id);
static int find_predecessor(node_t node, int id);
static int remote_find_predecessor(node_t node, int ask_to_id, int id);
static int remote_closest_preceding_finger(int ask_to_id, int id);
static void notify_predecessors(node_t node);
static void initialize_finger_table(node_t data, int known_id);
static void update_finger_table(node_t node, int candidate_id, int finger_index);
static void remote_update_finger_table(node_t node, int ask_to_id, int candidate_id, int finger_index);
static void notify_node_joined(node_t node, int new_node_id);
static void remote_notify_node_joined(node_t node, int notify_id);
*/

static void chord_initialize(void)
{
  // compute the powers of 2 once for all
  int pow = 1;
  int i;
  for (i = 0; i < NB_BITS; i++) {
    powers2[i] = pow;
    pow = pow << 1;
  }
}

/**
 * \brief Turns an id into an equivalent id in [0, NB_KEYS[
 * \param id an id
 * \return the corresponding normalized id
 */
static int normalize(int id)
{
  // make sure id >= 0
  while (id < 0) {
    id += NB_KEYS;
  }
  // make sure id < NB_KEYS
  id = id % NB_KEYS;

  return id;
}

/**
 * \brief Returns whether a id belongs to the interval [start, end].
 *
 * The parameters are noramlized to make sure they are between 0 and CHORD_NB_KEYS - 1).
 * 1 belongs to [62, 3]
 * 1 does not belong to [3, 62]
 * 63 belongs to [62, 3]
 * 63 does not belong to [3, 62]
 * 24 belongs to [21, 29]
 * 24 does not belong to [29, 21]
 *
 * \param id id to check
 * \param start lower bound
 * \param end upper bound
 * \return a non-zero value if id in in [start, end]
 */
static int is_in_interval(int id, int start, int end)
{
  id = normalize(id);
  start = normalize(start);
  end = normalize(end);

  // make sure end >= start and id >= start
  if (end < start) {
    end += NB_KEYS;
  }

  if (id < start) {
    id += NB_KEYS;
  }

  return id <= end;
}

/**
 * \brief Gets the mailbox name of a host given its chord id.
 * \param node_id id of a node
 * \return the name of its mailbox
 * FIXME: free the memory
 */
static char* get_mailbox(int node_id)
{
  return bprintf("mailbox%d", node_id);
}

/**
 * \brief Displays the finger table of a node.
 * \param node a node
 */
static void print_finger_table(node_t node)
{
  int i;
  int pow = 1;
  INFO0("My finger table:");
  INFO0("Start | Succ ");
  for (i = 0; i < NB_BITS; i++) {
    INFO2(" %3d  | %3d ", (node->id + pow) % NB_KEYS, node->fingers[i].id);
    pow = pow << 1;
  }
  INFO1("Predecessor: %d", node->pred_id);
}

/**
 * \brief Sets a finger of the current node.
 * \param node the current node
 * \param finger_index index of the finger to set (0 to NB_BITS - 1)
 * \param id the id to set for this finger
 */
static void set_finger(node_t node, int finger_index, int id)
{
  node->fingers[finger_index].id = id;
  xbt_free(node->fingers[finger_index].mailbox);
  node->fingers[finger_index].mailbox = get_mailbox(id);
  INFO2("My new finger #%d is %d", finger_index, id);
}

/**
 * \brief Sets the predecessor of the current node.
 * \param node the current node
 * \param id the id to predecessor, or -1 to unset the predecessor
 */
static void set_predecessor(node_t node, int predecessor_id)
{
  node->pred_id = predecessor_id;
  xbt_free(node->pred_mailbox);

  if (predecessor_id != -1) {
    node->pred_mailbox = get_mailbox(predecessor_id);
  }

  INFO1("My new predecessor is %d", predecessor_id);
}

/**
 * \brief Node Function
 * Arguments:
 * - my id
 * - the id of a guy I know in the system (except for the first node)
 * - the time to sleep before I join (except for the first node)
 */
int node(int argc, char *argv[])
{
  double init_time = MSG_get_clock();
  m_task_t task = NULL;
  msg_comm_t comm_send = NULL;
  msg_comm_t comm_receive = NULL;
  int i;
  char* mailbox = NULL;
  double deadline;
  double next_stabilize_date = init_time + 10;
  double next_fix_fingers_date = init_time + 10;
  double next_check_predecessor_date = init_time + 10;

  xbt_assert0(argc == 3 || argc == 5, "Wrong number of arguments for this node");

  // initialize my node
  s_node_t node = {0};
  node.id = atoi(argv[1]);
  node.mailbox = get_mailbox(node.id);
  node.next_finger_to_fix = 0;
  node.comms = xbt_dynar_new(sizeof(msg_comm_t), NULL);

  for (i = 0; i < NB_BITS; i++) {
    set_finger(&node, i, node.id);
  }

  if (argc == 3) { // first ring
    deadline = atof(argv[2]);
    create(&node);
  }
  else {
    int known_id = atoi(argv[2]);
    double sleep_time = atof(argv[3]);
    deadline = atof(argv[4]);

    // sleep before starting
    INFO1("Let's sleep during %f", sleep_time);
    MSG_process_sleep(sleep_time);
    INFO0("Hey! Let's join the system.");

    join(&node, known_id);
  }

  while (MSG_get_clock() < init_time + deadline) {

    // see if some communications are finished
    int index;
    while ((index = MSG_comm_testany(node.comms)) != -1) {
      comm_send = xbt_dynar_get_as(node.comms, index, msg_comm_t);
      xbt_dynar_remove_at(node.comms, index, &comm_send);
      MSG_comm_destroy(comm_send);
    }

    if (comm_receive == NULL) {
      task = NULL;
      comm_receive = MSG_task_irecv(&task, node.mailbox);
    }

    if (!MSG_comm_test(comm_receive)) {

      // no task was received: make some periodic calls
      if (MSG_get_clock() >= next_stabilize_date) {
        stabilize(&node);
        next_stabilize_date = MSG_get_clock() + 10;
      }
      else if (MSG_get_clock() >= next_fix_fingers_date) {
        fix_fingers(&node);
        next_fix_fingers_date = MSG_get_clock() + 10;
      }
      else if (MSG_get_clock() >= next_check_predecessor_date) {
        check_predecessor(&node);
        next_check_predecessor_date = MSG_get_clock() + 10;
      }
      else {
	// nothing to do: sleep for a while
        MSG_process_sleep(5);
      }
    }
    else {
      // a task was received

      MSG_comm_destroy(comm_receive);
      comm_receive = NULL;
      const char* task_name = MSG_task_get_name(task);
      task_data_t task_data = (task_data_t) MSG_task_get_data(task);

      if (!strcmp(task_name, "Find Successor")) {
        INFO2("Receiving a 'Find Successor' Request from %s for id %d",
              task_data->issuer_host_name, task_data->request_id);
	// is my successor the successor?
	if (is_in_interval(task_data->request_id, node.id + 1, node.fingers[0].id)) {
	  task_data->answer_id = node.fingers[0].id;
	  MSG_task_set_name(task, "Find Successor Answer");
	  INFO3("Sending back a 'Find Successor Answer' to %s: the successor of %d is %d",
	        task_data->issuer_host_name, task_data->request_id, task_data->answer_id);
	  comm_send = MSG_task_isend(task, task_data->answer_to);
	  xbt_dynar_push(node.comms, &comm_send);
	}
	else {
	  // otherwise, forward the request to the closest preceding finger in my table
	  int closest = closest_preceding_node(&node, task_data->request_id);
	  INFO2("Forwarding the 'Find Successor' request for id %d to my closest preceding finger %d",
	        task_data->request_id, closest);
	  mailbox = get_mailbox(closest);
	  comm_send = MSG_task_isend(task, mailbox);
	  xbt_dynar_push(node.comms, &comm_send);
	  xbt_free(mailbox);
	}
      }

      else if (!strcmp(task_name, "Get Predecessor")) {
	INFO1("Receiving a 'Get Predecessor' Request from %s", task_data->issuer_host_name);
	task_data->answer_id = node.pred_id;
	MSG_task_set_name(task, "Get Predecessor Answer");
	INFO2("Sending back a 'Get Predecessor Answer' to %s: my predecessor is %d",
	      task_data->issuer_host_name, task_data->answer_id);
	comm_send = MSG_task_isend(task, task_data->answer_to);
	xbt_dynar_push(node.comms, &comm_send);
      }
      /*
      else if (!strcmp(task_name, "Find Predecessor")) {
	INFO2("Receiving a 'Find Predecessor' Request from %s for id %d", task_data->issuer_host_name, task_data->request_id);
	// am I the predecessor?
	if (is_in_interval(task_data->request_id, node.id + 1, node.fingers[0].id)) {
	  task_data->answer_id = node.id;
	  MSG_task_set_name(task, "Find Predecessor Answer");
	  INFO3("Sending back a 'Find Predecessor' Answer to %s: the predecessor of %d is %d", task_data->issuer_host_name, task_data->request_id, task_data->answer_id);
	  comm = MSG_task_isend(task, task_data->answer_to);
	  xbt_dynar_push(node.comms, &comm);
	}
	else {
	  // otherwise, forward the request to the closest preceding finger in my table
	  int closest = closest_preceding_node(&node, task_data->request_id);
	  INFO2("Forwarding 'Find Predecessor' request for id %d to my closest preceding finger %d", task_data->request_id, closest);
	  mailbox = get_mailbox(closest);
	  comm = MSG_task_isend(task, mailbox);
	  xbt_dynar_push(node.comms, &comm);
	  xbt_free(mailbox);
	}
      }
      */

      /*
      else if (!strcmp(task_name, "Notify Node Joined")) {
	// someone may be my new neighboor
	INFO1("Receiving a 'Notify Node Joine' request from %s", task_data->issuer_host_name);
	notify_node_joined(&node, task_data->request_id);
      }
      else if (!strcmp(task_name, "Update Finger")) {
	// someone is telling me that he may be my new finger
	INFO1("Receiving an 'Update Finger' request from %s", task_data->issuer_host_name);
	update_finger_table(&node, task_data->request_id, task_data->request_finger);
      }
      */
      else if (!strcmp(task_name, "Notify")) {
	// someone is telling me that he may be my new predecessor
	INFO1("Receiving a 'Notify' request from %s", task_data->issuer_host_name);
	notify(&node, task_data->request_id);
      }
      else if (!strcmp(task_name, "Predecessor Leaving")) {
	// my predecessor is about to quit
	INFO1("Receiving a 'Predecessor Leaving' message from %s", task_data->issuer_host_name);
	// modify my predecessor
	set_predecessor(&node, task_data->pred_id);
	/*TODO :
          >> notify my new predecessor
          >> send a notify_predecessors !!
	 */
      }
      else if (!strcmp(task_name, "Successor Leaving")) {
	// my successor is about to quit
	INFO1("Receiving a 'Successor Leaving' message from %s", task_data->issuer_host_name);
	// modify my successor FIXME : this should be implicit ?
	set_finger(&node, 0, task_data->successor_id);
	/* TODO
          >> notify my new successor
          >> update my table & predecessors table */
      }
    }
  }

  // leave the ring and stop the simulation
  leave(&node);
  xbt_dynar_free(&node.comms);
  xbt_free(node.mailbox);
  xbt_free(node.pred_mailbox);
  for (i = 0; i < NB_BITS - 1; i++) {
    xbt_free(node.fingers[i].mailbox);
  }
  return 0;
}

/**
 * \brief Initializes the current node as the first one of the system.
 * \param node the current node
 */
static void create(node_t node)
{
  INFO0("Create a new Chord ring...");
  set_predecessor(node, -1); // -1 means that I have no predecessor
  print_finger_table(node);
}

/**
 * \brief Makes the current node join the ring, knowing the id of a node
 * already in the ring
 * \param node the current node
 * \param known_id id of a node already in the ring
 */
static void join(node_t node, int known_id)
{
  INFO2("Joining the ring with id %d, knowing node %d", node->id, known_id);
  set_predecessor(node, -1); // no predecessor (yet)

  int successor_id;
  do {
    successor_id = remote_find_successor(node, known_id, node->id);
    INFO0("My 'Find Successor' request has failed, let's try again");
  } while (successor_id == -1);

  set_finger(node, 0, successor_id);
  print_finger_table(node);
}

/**
 * \brief Notifies the current node that a node has just joined the network.
 * \param node the current node
 * \param new_node_id id of the new node in the network
 */
/*
static void notify_node_joined(node_t node, int new_node_id)
{
  INFO1("A new node %d has joined the network, let's see if it is a neighboor", new_node_id);
  if (is_in_interval(new_node_id, node->id + 1, node->fingers[0].id - 1)) {
    // new_node_id is my new successor
    set_finger(node, 0, new_node_id);
    bootstrap(node, new_node_id);
  }

  if (is_in_interval(new_node_id, node->pred_id + 1, node->id - 1)) {
    // new_node_id is my new predecessor
    set_predecessor(node, new_node_id);
    bootstrap(node, new_node_id);
  }
}
*/

/**
 * \brief Notifies a remote node that the current node has just joined
 * the network.
 * \param node the current node
 * \param notify_id id of the remote node to notify
 */
/*
static void remote_notify_node_joined(node_t node, int notify_id)
{
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->request_id = node->id;
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Notify" request to notify_id
  INFO2("Sending a 'Notify' request to %d because I have joined the network with id %d", notify_id, node->id);
  m_task_t task = MSG_task_create("Notify", 1000, 5000, req_data);
  char* mailbox = get_mailbox(notify_id);
  msg_comm_t comm = MSG_task_isend(task, mailbox);
  xbt_dynar_push(node->comms, &comm);
  xbt_free(mailbox);
}
*/

/**
* \brief Let the current node send queries to fill in its own finger table.
* \param node the current node
* \param ask_to_id id of a node to send queries to
*/
/*
static void bootstrap(node_t node, int ask_to_id)
{
  INFO0("Filling my finger table");
  int i, pred_id, succ_id;
  int pow = 1;

  for (i = 0; i < NB_BITS; i++)
  {
    pred_id = remote_find_successor(node, ask_to_id, pow);
    do {
      succ_id = pred_id;
      pred_id = remote_find_predecessor(node, pred_id, pred_id);
    } while (pred_id >= pow);

    pow = pow << 1;
    set_finger(node, i, succ_id);
  }
}
*/

/**
 * \brief Makes the current node quit the system
 * \param node the current node
 */
static void leave(node_t node)
{
  INFO0("Well Guys! I Think it's time for me to quit ;)");
  quit_notify(node, 1);  // notify to my successor ( >>> 1 );
  quit_notify(node, -1); // notify my predecessor  ( >>> -1);
  // TODO ...
}

/*
 * \brief Notifies the successor or the predecessor of the current node
 * of the departure
 * \param node the current node
 * \param to 1 to notify the successor, -1 to notify the predecessor
 * FIXME: notify both nodes with only one call
 */
static void quit_notify(node_t node, int to)
{
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->request_id = node->id;
  req_data->successor_id = node->fingers[0].id;
  req_data->pred_id = node->pred_id;
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());
  const char *task_name = NULL;
  const char* to_mailbox = NULL;
  if (to == 1)    // notify my successor
  {
    to_mailbox = node->fingers[0].mailbox;
    INFO2("Telling my Successor %d about my departure via mailbox %s",
	  node->fingers[0].id, to_mailbox);
    task_name = "Predecessor Leaving";

  }
  else if (to == -1)    // notify my predecessor
  {
    if (node->pred_id == -1) {
      return;
    }

    to_mailbox = node->pred_mailbox;
    INFO2("Telling my Predecessor %d about my departure via mailbox %s",
	  node->pred_id, to_mailbox);
    task_name = "Predecessor Leaving";

  }
  m_task_t task = MSG_task_create(task_name, 1000, 5000, req_data);
  //char* mailbox = get_mailbox(to_mailbox);
  msg_comm_t comm = MSG_task_isend(task, to_mailbox);
  xbt_dynar_push(node->comms, &comm);
}

/*
 * \brief Initializes my finger table, knowing the id of a node already in the system.
 * \param node the current node
 * \param known_id id of a node already in the system
 */
/*
static void initialize_finger_table(node_t node, int known_id)
{
  int my_id = node->id;
  int i;
  int pow = 1; // 2^i

  INFO0("Initializing my finger table...");

  // ask known_id who is my immediate successor
  node->fingers[0].id = remote_find_successor(node, known_id, my_id + 1);
  node->fingers[0].mailbox = get_mailbox(node->fingers[0].id);

  // find all other fingers
  for (i = 0; i < NB_BITS - 1; i++) {

    pow = pow << 1; // equivalent to pow = pow * 2
    if (is_in_interval(my_id + pow, my_id, node->fingers[i].id - 1)) {
      // I already have the info for this finger
      node->fingers[i + 1].id = node->fingers[i].id;
    }
    else {
      // I don't have the info, ask the only guy I know
      node->fingers[i + 1].id = remote_find_successor(node, known_id, my_id + pow);
    }
    node->fingers[i + 1].mailbox = get_mailbox(node->fingers[i + 1].id);
  }

  node->pred_id = find_predecessor(node, node->id);
  node->pred_mailbox = get_mailbox(node->pred_id);

  INFO0("Finger table initialized!");
  print_finger_table(node);
}
*/

/**
 * \brief Notifies some nodes that the current node may have became their finger.
 * \param node the current node, which has just joined the system
 */
/*
static void notify_predecessors(node_t node)
{
  int i, pred_id;
  int pow = 1;
  for (i = 0; i < NB_BITS; i++) {
    // find the closest node whose finger #i can be me
    pred_id = find_predecessor(node, node->id - pow + 1); // note: no "+1" in the article!
    if (pred_id != node->id) {
      remote_update_finger_table(node, pred_id, node->id, i);
    }
    pow = pow << 1; // pow = pow * 2
  }
}
*/

/**
 * \brief Tells the current node that a node may have became its new finger.
 * \param node the current node
 * \param candidate_id id of the node that may be a new finger of the current node
 * \param finger_index index of the finger to update
 */
/*
static void update_finger_table(node_t node, int candidate_id, int finger_index)
{
  int pow = 1;
  int i;
  for (i = 0; i < finger_index; i++) {
    pow = pow << 1;
  }

  //  if (is_in_interval(candidate_id, node->id + pow, node->fingers[finger_index].id - 1)) {
  if (is_in_interval(candidate_id, node->id, node->fingers[finger_index].id - 1)) {
//    INFO3("Candidate %d is between %d and %d!", candidate_id, node->id + pow, node->fingers[finger_index].id - 1);
    // candidate_id is my new finger
    set_finger(node, finger_index, candidate_id);
    print_finger_table(node);

    if (node->pred_id != node->id) { // FIXME: is this necessary?
      // my predecessor may be concerned too
      remote_update_finger_table(node, node->pred_id, candidate_id, finger_index);
    }
  }
}
*/

/**
 * \brief Tells a remote node that a node may have became its new finger.
 * \param ask_to_id id of the remote node to update
 * \param candidate_id id of the node that may be a new finger of the remote node
 * \param finger_index index of the finger to update
 */
/*
static void remote_update_finger_table(node_t node, int ask_to_id, int candidate_id, int finger_index)
{
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->request_id = candidate_id;
  req_data->request_finger = finger_index;
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Update Finger" request to ask_to_id
  INFO3("Sending an 'Update Finger' request to %d: his finger #%d may be %d now", ask_to_id, finger_index, candidate_id);
  m_task_t task = MSG_task_create("Update Finger", 1000, 5000, req_data);
  char* mailbox = get_mailbox(ask_to_id);
  msg_comm_t comm = MSG_task_isend(task, mailbox);
  xbt_dynar_push(node->comms, &comm);
  xbt_free(mailbox);
}
*/

/**
 * \brief Makes the current node find the successor node of an id.
 * \param node the current node
 * \param id the id to find
 * \return the id of the successor node, or -1 if the request failed
 */
static int find_successor(node_t node, int id)
{
  // is my successor the successor?
  if (is_in_interval(id, node->id + 1, node->fingers[0].id)) {
    return node->fingers[0].id;
  }

  // otherwise, ask the closest preceding finger in my table
  int closest = closest_preceding_node(node, id);
  return remote_find_successor(node, closest, id);
}

/**
 * \brief Asks another node the successor node of an id.
 * \param node the current node
 * \param ask_to the node to ask to
 * \param id the id to find
 * \return the id of the successor node, or -1 if the request failed
 */
static int remote_find_successor(node_t node, int ask_to, int id)
{
  int successor = -1;
  s_task_data_t req_data;
  char* mailbox = bprintf("%s Find Successor", node->mailbox);
  req_data.request_id = id;
  req_data.answer_to = mailbox;
  req_data.issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Find Successor" request to ask_to_id
  INFO2("Sending a 'Find Successor' request to %d for key %d", ask_to, id);
  m_task_t task = MSG_task_create("Find Successor", 1000, 5000, &req_data);
  MSG_error_t res = MSG_task_send_with_timeout(task, get_mailbox(ask_to), 20);
  if (res == MSG_OK) {

    // receive the answer
    task = NULL;
    res = MSG_task_receive_with_timeout(&task, req_data.answer_to, 20);

    if (res == MSG_OK) {

      task_data_t ans_data;
      ans_data = MSG_task_get_data(task);
      successor = ans_data->answer_id;
      INFO2("Received the answer to my 'Find Successor' request: the successor of key %d is %d", id, successor);
    }
  }

  xbt_free(mailbox);
  return successor;
}

/**
 * \brief Asks another node its predecessor.
 * \param node the current node
 * \param ask_to the node to ask to
 * \return the id of its predecessor node, or -1 if the request failed
 * (or if the node does not know its predecessor)
 */
static int remote_get_predecessor(node_t node, int ask_to)
{
  int predecessor_id = -1;
  s_task_data_t req_data;
  char* mailbox = bprintf("%s Get Predecessor", node->mailbox);
  req_data.answer_to = mailbox;
  req_data.issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Get Predecessor" request to ask_to_id
  INFO1("Sending a 'Get Predecessor' request to %d", ask_to);
  m_task_t task = MSG_task_create("Get Predecessor", 1000, 5000, &req_data);
  MSG_error_t res = MSG_task_send_with_timeout(task, get_mailbox(ask_to), 20);

  if (res == MSG_OK) {

    // receive the answer
    task = NULL;
    res = MSG_task_receive_with_timeout(&task, req_data.answer_to, 20);

    if (res == MSG_OK) {

      task_data_t ans_data;
      ans_data = MSG_task_get_data(task);
      predecessor_id = ans_data->answer_id;
      INFO2("Received the answer to my 'Get Predecessor' request: the predecessor of node %d is %d", ask_to, predecessor_id);
    }
  }

  xbt_free(mailbox);
  return predecessor_id;
}

/**
 * \brief Makes the current node find the predecessor node of an id.
 * \param node the current node
 * \param id the id to find
 * \return the id of the predecessor node
 */
/*
static int find_predecessor(node_t node, int id)
{
  if (node->id == node->fingers[0].id) {
    // I am the only node in the system
    return node->id;
  }

  if (is_in_interval(id, node->id + 1, node->fingers[0].id)) {
    return node->id;
  }
  int ask_to = closest_preceding_node(node, id);
  return remote_find_predecessor(node, ask_to, id);
}
*/

/**
 * \brief Asks another node the predecessor node of an id.
 * \param node the current node
 * \param ask_to the node to ask to
 * \param id the id to find
 * \return the id of the predecessor node
 */
/*
static int remote_find_predecessor(node_t node, int ask_to, int id)
{
  s_task_data_t req_data;
  char* mailbox = bprintf("%s Find Predecessor", node->mailbox);
  req_data.request_id = id;
  req_data.answer_to = mailbox;
  req_data.issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Find Predecessor" request to ask_to
  INFO2("Sending a 'Find Predecessor' request to %d for key %d", ask_to, id);
  m_task_t task = MSG_task_create("Find Predecessor", 1000, 5000, &req_data);
  MSG_task_send(task, get_mailbox(ask_to));

  // receive the answer
  task = NULL;
  MSG_task_receive(&task, req_data.answer_to);
  task_data_t ans_data;
  ans_data = MSG_task_get_data(task);
  int predecessor = ans_data->answer_id;
  xbt_free(mailbox);
  INFO2("Received the answer to my 'Find Predecessor' request: the predecessor of key %d is %d", id, predecessor);

  return predecessor;
}
*/

/**
 * \brief Returns the closest preceding finger of an id
 * with respect to the finger table of the current node.
 * \param node the current node
 * \param id the id to find
 * \return the closest preceding finger of that id
 */
int closest_preceding_node(node_t node, int id)
{
  int i;
  for (i = NB_BITS - 1; i >= 0; i--) {
    if (is_in_interval(node->fingers[i].id, node->id + 1, id - 1)) {
      return node->fingers[i].id;
    }
  }
  return node->id;
}

/**
 * \brief This function is called periodically. It checks the immediate
 * successor of the current node.
 * \param node the current node
 */
static void stabilize(node_t node)
{
  INFO0("Stabilizing node");

  // get the predecessor of my immediate successor
  int candidate_id;
  int successor_id = node->fingers[0].id;
  if (successor_id != node->id) {
    candidate_id = remote_get_predecessor(node, successor_id);
  }
  else {
    candidate_id = node->pred_id;
  }

  // this node is a candidate to become my new successor
  if (candidate_id != -1
      && is_in_interval(candidate_id, node->id + 1, successor_id - 1)) {
    set_finger(node, 0, candidate_id);
  }
  if (successor_id != node->id) {
    remote_notify(node, successor_id, node->id);
  }
}

/**
 * \brief Notifies the current node that its predecessor may have changed.
 * \param node the current node
 * \param candidate_id the possible new predecessor
 */
static void notify(node_t node, int predecessor_candidate_id) {

  if (node->pred_id == -1
    || is_in_interval(predecessor_candidate_id, node->pred_id + 1, node->id - 1)) {

    set_predecessor(node, predecessor_candidate_id);
    print_finger_table(node);
  }
  else {
    INFO1("I don't have to change my predecessor to %d", predecessor_candidate_id);
  }
}

/**
 * \brief Notifies a remote node that its predecessor may have changed.
 * \param node the current node
 * \param notify_id id of the node to notify
 * \param candidate_id the possible new predecessor
 */
static void remote_notify(node_t node, int notify_id, int predecessor_candidate_id) {

  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->request_id = predecessor_candidate_id;
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Notify" request to notify_id
  INFO1("Sending a 'Notify' request to %d", notify_id);
  m_task_t task = MSG_task_create("Notify", 1000, 5000, req_data);
  char* mailbox = get_mailbox(notify_id);
  msg_comm_t comm = MSG_task_isend(task, mailbox);
  xbt_dynar_push(node->comms, &comm);
  xbt_free(mailbox);
}

/**
 * \brief This function is called periodically.
 * It refreshes the finger table of the current node.
 * \param node the current node
 */
static void fix_fingers(node_t node) {

  INFO0("Fixing fingers");
  int i = node->next_finger_to_fix;
  int id = find_successor(node, node->id + powers2[i]);
  if (id != -1) {

    if (id != node->fingers[i].id) {
      set_finger(node, i, id);
      print_finger_table(node);
    }
    node->next_finger_to_fix = (i + 1) % NB_BITS;
  }
}

/**
 * \brief This function is called periodically.
 * It checks whether the predecessor has failed
 * \param node the current node
 */
static void check_predecessor(node_t node)
{
  INFO0("Checking whether my predecessor is alive");
  // TODO
}

/**
 * \brief Main function.
 */
int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s ../msg_platform.xml chord.xml\n", argv[0]);
    exit(1);
  }

  const char* platform_file = argv[1];
  const char* application_file = argv[2];

  chord_initialize();

  MSG_global_init(&argc, argv);
  MSG_set_channel_number(0);
  MSG_create_environment(platform_file);

  MSG_function_register("node", node);
  MSG_launch_application(application_file);

  MSG_error_t res = MSG_main();
  INFO1("Simulation time: %g", MSG_get_clock());

  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
