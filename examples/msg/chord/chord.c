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
  const char* mailbox;
} s_finger_t, *finger_t;

/**
 * Node data.
 */
typedef struct node {
  int id;                                 // my id
  const char* mailbox;
  s_finger_t fingers[NB_BITS];            // finger table (fingers[0] is my successor)
  int pred_id;                            // predecessor id
  const char* pred_mailbox;
} s_node_t, *node_t;

/**
 * Task data
 */
typedef struct task_data {
  int request_id;
  int request_finger;
  int answer_id;
  const char* answer_to;
} s_task_data_t, *task_data_t;

// utility functions
static int normalize(int id);
static int is_in_interval(int id, int start, int end);
static char* get_mailbox(int host_id);

// process functions
static int node(int argc, char *argv[]);
static int sender(int argc,char *argv[]);

// initialization
static void initialize_first_node(node_t node);
static void initialize_finger_table(node_t data, int known_id);
static void join(node_t node, int known_id);

// Chord core
static int find_successor(node_t node, int id);
static int remote_find_successor(node_t node, int ask_to_id, int id);
static int find_predecessor(node_t node, int id);
static int remote_find_predecessor(node_t node, int ask_to_id, int id);
static int closest_preceding_finger(node_t node, int id);
static int remote_closest_preceding_finger(int ask_to_id, int id);
static void notify(node_t);
static void remote_move_keys(node_t node, int take_from_id);
static void update_finger_table(node_t node, int candidate_id, int finger_index);
static void remote_update_finger_table(int ask_to_id, int candidate_id, int finger_index);

//static void find_successor_node(node_t my_data, m_task_t join_task);

/**
 * \brief Turns an id into an equivalent id in [0, NB_KEYS[
 * \param id an id
 * \return the corresponding normalized id
 */
static int normalize(int id) {

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
static int is_in_interval(int id, int start, int end) {

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
static char* get_mailbox(int node_id) {

  return bprintf("mailbox%d", node_id);
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
  xbt_assert0(argc == 2 || argc == 4, "Wrong number of arguments for this node");

  // initialize my node
  s_node_t node = {0};
  node.id = atoi(argv[1]);
  node.mailbox = get_mailbox(node.id);

  if (argc == 2) { // first ring
    initialize_first_node(&node);
  }
  else {
    int known_id = atoi(argv[2]);
    double sleep_time = atof(argv[3]);

    // sleep before starting
    INFO1("Let's sleep >>%f", sleep_time);
    MSG_process_sleep(sleep_time);
    INFO0("Hey! Let's join the system.");

    join(&node, known_id);
  }

  while (1) {

    m_task_t task = NULL;
    MSG_error_t res = MSG_task_receive(&task, node.mailbox);

    xbt_assert0(res == MSG_OK, "MSG_task_receive failed");

    // get data
    const char* task_name = MSG_task_get_name(task);
    task_data_t task_data = (task_data_t) MSG_task_get_data(task);

    if (!strcmp(task_name, "Find Successor")) {

      // is my successor the successor?
      if (is_in_interval(task_data->request_id, node.id + 1, node.fingers[0].id)) {
	task_data->answer_id = node.fingers[0].id;
        MSG_task_set_name(task, "Find Successor Answer");
        MSG_task_send(task, task_data->answer_to);
      }
      else {
	// otherwise, forward the request to the closest preceding finger in my table
	int closest = closest_preceding_finger(&node, task_data->request_id);
	MSG_task_send(task, get_mailbox(closest));
      }
    }
    /*
    else if (!strcmp(task_name, "Find Successor Answer")) {

    }
    */
    else if (!strcmp(task_name, "Find Predecessor")) {

      // am I the predecessor?
      if (is_in_interval(task_data->request_id, node.id + 1, node.fingers[0].id)) {
	task_data->answer_id = node.id;
        MSG_task_set_name(task, "Find Predecessor Answer");
        MSG_task_send(task, task_data->answer_to);
      }
      else {
	// otherwise, forward the request to the closest preceding finger in my table
	int closest = closest_preceding_finger(&node, task_data->request_id);
	MSG_task_send(task, get_mailbox(closest));
      }
    }
    /*
    else if (!strcmp(task_name, "Find Predecessor Answer")) {

    }
    */
    else if (!strcmp(task_name, "Update Finger")) {
      update_finger_table(&node, task_data->request_id, task_data->request_finger);
    }
    /*
    else if (!strcmp(task_name, "Fix Fingers"))
    {
      int i;
      for (i = KEY_BITS - 1 ; i >= 0; i--)
      {
	data->fingers[i] = find_finger_elem(data,(data->id)+pow(2,i-1));
      }
    }
    */
  }
}

/**
 * \brief Initializes the current node as the first one of the system.
 * \param node the current node
 */
static void initialize_first_node(node_t node)
{
  INFO0("Create a new Chord ring...");

  // I am my own successor and predecessor
  int i;
  for (i = 0; i < NB_BITS; i++) {
    node->fingers[i].id = node->id;
    node->fingers[i].mailbox = node->mailbox;
  }
  node->pred_id = node->id;
  node->pred_mailbox = node->mailbox;
}

/**
 * \brief Makes the current node join the system, knowing the id of a node already in the system
 * \param node the current node
 * \param known_id id of a node already in the system
 */
static void join(node_t node, int known_id)
{
  initialize_finger_table(node, known_id); // determine my fingers, asking to known_id
  notify(node); // tell others that I may have became their finger
  remote_move_keys(node, node->fingers[0].id); // take some key-value pairs from my sucessor
}

/*
 * \brief Initializes my finger table, knowing the id of a node already in the system.
 * \param node the current node
 * \param known_id id of a node already in the system
 */
static void initialize_finger_table(node_t node, int known_id)
{
 int my_id = node->id;
 int i;
 int pow = 1; // 2^i

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
}

/**
 * \brief Notifies some nodes that the current node may have became their finger.
 * \param node the current node, which has just joined the system
 */
static void notify(node_t node)
{
  int i, pred_id;
  int pow = 1;
  for (i = 0; i < NB_KEYS; i++) {
    // find the closest node whose finger #i can be me
    pred_id = find_predecessor(node, node->id - pow);
    remote_update_finger_table(pred_id, node->id, i);
    pow = pow << 1; // pow = pow * 2
  }
}

/**
 * \brief Tells the current node that a node may have became its new finger.
 * \param node the current node
 * \param candidate_id id of the node that may be a new finger of the current node
 * \param finger_index index of the finger to update
 */
static void update_finger_table(node_t node, int candidate_id, int finger_index)
{
  if (is_in_interval(candidate_id, node->id, node->fingers[finger_index].id - 1)) {

    // candidate_id is my new finger
    node->fingers[finger_index].id = candidate_id;
    node->fingers[finger_index].mailbox = get_mailbox(candidate_id);

    // my predecessor may be concerned too
    remote_update_finger_table(node->pred_id, candidate_id, finger_index);
  }
}

/**
 * \brief Tells a remote node that a node may have became its new finger.
 * \param ask_to_id id of the remote node to update
 * \param candidate_id id of the node that may be a new finger of the remote node
 * \param finger_index index of the finger to update
 */
static void remote_update_finger_table(int ask_to_id, int candidate_id, int finger_index)
{
  s_task_data_t req_data;
  req_data.request_id = candidate_id;
  req_data.request_finger = finger_index;

  // send a "Update Finger" request to ask_to_id
  m_task_t task = MSG_task_create("Update Finger", 1000, 5000, &req_data);
  MSG_task_send(task, get_mailbox(ask_to_id));
}

/* deprecated version where the remote host modifies the issuer's node data
static void find_successor_node(node_t n_data,m_task_t join_task)  //use all data
{
	//get recv data
	node_t recv_data = (node_t)MSG_task_get_data(join_task);
	INFO3("recv_data->id : %i , n_data->id :%i , successor->id :%i",recv_data->id,n_data->id,n_data->fingers[0].id);
	//if ((recv_data->id >= n_data->id) && (recv_data->id <= n_data->fingers[0].id))
	if (is_in_interval(recv_data->id,n_data->id,n_data->fingers[0].id))
	{
		INFO1("Successor Is %s",n_data->fingers[0].host_name);
		//predecessor(recv_data) >>>> n_data
		recv_data->pred_host_name = n_data->host_name;
		recv_data->pred_id = n_data->id;
		recv_data->pred_mailbox = n_data->pred_mailbox;
		// successor(recv_data) >>>> n_data.finger[0]
		recv_data->fingers_nb = 1;
		recv_data->fingers[0].host_name = n_data->fingers[0].host_name;
		recv_data->fingers[0].id = n_data->fingers[0].id;
		recv_data->fingers[0].mailbox = n_data->fingers[0].mailbox;
		// successor(n_data) >>>> recv_sucessor
		n_data->fingers[0].id = recv_data->id;
		n_data->fingers[0].host_name = recv_data->host_name;
		n_data->fingers[0].mailbox = recv_data->mailbox;
		// Logs
		INFO1("Sending back a Join Request to %s",recv_data->host_name);
		MSG_task_set_name(join_task,"Join Response");
		MSG_task_send(join_task,recv_data->mailbox);
	}

	else
	{
		const char* closest_preceding_mailbox = find_closest_preceding(n_data,recv_data->id);
		INFO1("Forwarding Join Call to mailbox %s",closest_preceding_mailbox);
		MSG_task_send(join_task,closest_preceding_mailbox);
	}
}
*/

/**
 * \brief Makes the current node find the successor node of an id.
 * \param node the current node
 * \param id the id to find
 * \return the id of the successor node
 */
static int find_successor(node_t node, int id)
{
  // is my successor the successor?
  if (is_in_interval(id, node->id + 1, node->fingers[0].id)) {
    return node->fingers[0].id;
  }

  // otherwise, ask the closest preceding finger in my table
  int closest = closest_preceding_finger(node, id);
  return remote_find_successor(node, closest, id);
}

/**
 * \brief Asks another node the successor node of an id.
 * \param node the current node
 * \param ask_to the node to ask to
 * \param id the id to find
 * \return the id of the successor node
 */
static int remote_find_successor(node_t node, int ask_to, int id)
{
  s_task_data_t req_data;
  req_data.request_id = id;
  req_data.answer_to = node->mailbox;

  // send a "Find Successor" request to ask_to_id
  m_task_t task = MSG_task_create("Find Successor", 1000, 5000, &req_data);
  MSG_task_send(task, get_mailbox(ask_to));

  // receive the answer
  task = NULL;
  MSG_task_receive(&task, node->mailbox); // FIXME: don't receive here other messages than the excepted one
  task_data_t ans_data;
  ans_data = MSG_task_get_data(task);
  int successor = ans_data->answer_id;

  return successor;
}

/**
 * \brief Makes the current node find the predecessor node of an id.
 * \param node the current node
 * \param id the id to find
 * \return the id of the predecessor node
 */
static int find_predecessor(node_t node, int id)
{
  if (node->id == node->fingers[0].id) {
    // I am the only node in the system
    return node->id;
  }

  if (is_in_interval(id, node->id + 1, node->fingers[0].id)) {
    return node->id;
  }
  int ask_to = closest_preceding_finger(node, id);
  return remote_find_predecessor(node, ask_to, id);
}

/**
 * \brief Asks another node the predecessor node of an id.
 * \param node the current node
 * \param ask_to the node to ask to
 * \param id the id to find
 * \return the id of the predecessor node
 */
static int remote_find_predecessor(node_t node, int ask_to, int id)
{
  s_task_data_t req_data;
  req_data.request_id = id;
  req_data.answer_to = node->mailbox;

  // send a "Find Predecessor" request to ask_to
  m_task_t task = MSG_task_create("Find Predecessor", 1000, 5000, &req_data);
  MSG_task_send(task, get_mailbox(ask_to));

  // receive the answer
  task = NULL;
  MSG_task_receive(&task, node->mailbox); // FIXME: don't receive here other messages than the excepted one
  task_data_t ans_data;
  ans_data = MSG_task_get_data(task);
  int predecessor = ans_data->answer_id;

  return predecessor;
}

/**
 * \brief Returns the closest preceding finger of an id
 * with respect to the finger table of the current node.
 * \param node the current node
 * \param id the id to find
 * \return the closest preceding finger of that id
 */
int closest_preceding_finger(node_t node, int id)
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
 * \brief Asks a node to take some of its keys.
 * \param node the current node, which has just joined the system
 * \param take_from_id id of a node who may have keys to give to the current node
 */
static void remote_move_keys(node_t node, int take_from_id) {
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

  MSG_global_init(&argc, argv);

  const char* platform_file = argv[1];
  const char* application_file = argv[2];

  /* MSG_config("workstation/model","KCCFLN05"); */
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
