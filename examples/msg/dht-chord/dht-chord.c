/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/modelchecker.h"
#include <xbt/RngStream.h>
#include "src/mc/mc_replay.h" // FIXME: this is an internal header

/** @addtogroup MSG_examples
 *
 *  - <b>chord/chord.c: Classical Chord P2P protocol</b>
 *    This example implements the well known Chord P2P protocol. Its main advantage is that it constitute a fully
 *    working non-trivial example. In addition, its implementation is rather efficient, as demonstrated in
 *    http://hal.inria.fr/inria-00602216/
 */


XBT_LOG_NEW_DEFAULT_CATEGORY(msg_chord, "Messages specific for this msg example");

#define COMM_SIZE 10
#define COMP_SIZE 0
#define MAILBOX_NAME_SIZE 10

static int nb_bits = 24;
static int nb_keys = 0;
static int timeout = 50;
static int max_simulation_time = 1000;
static int periodic_stabilize_delay = 20;
static int periodic_fix_fingers_delay = 120;
static int periodic_check_predecessor_delay = 120;
static int periodic_lookup_delay = 10;

static const double sleep_delay = 4.9999;

extern long int smx_total_comms;

/* Finger element. */
typedef struct s_finger {
  int id;
  char mailbox[MAILBOX_NAME_SIZE]; // string representation of the id
} s_finger_t, *finger_t;

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
} s_node_t, *node_t;

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
} s_task_data_t, *task_data_t;

static int *powers2;
static xbt_dynar_t host_list;

// utility functions
static void chord_exit(void);
static int normalize(int id);
static int is_in_interval(int id, int start, int end);
static void get_mailbox(int host_id, char* mailbox);
static void task_free(void* task);
static void print_finger_table(node_t node);
static void set_finger(node_t node, int finger_index, int id);
static void set_predecessor(node_t node, int predecessor_id);

// process functions
static int node(int argc, char *argv[]);
static void handle_task(node_t node, msg_task_t task);

// Chord core
static void create(node_t node);
static int join(node_t node, int known_id);
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
static void random_lookup(node_t);
static void quit_notify(node_t node);

/* \brief Global initialization of the Chord simulation. */
static void chord_initialize(void)
{
  // compute the powers of 2 once for all
  powers2 = xbt_new(int, nb_bits);
  int pow = 1;
  unsigned i;
  for (i = 0; i < nb_bits; i++) {
    powers2[i] = pow;
    pow = pow << 1;
  }
  nb_keys = pow;
  XBT_DEBUG("Sets nb_keys to %d", nb_keys);

  msg_host_t host;
  host_list = MSG_hosts_as_dynar();
  xbt_dynar_foreach(host_list, i, host) {
    char descr[512];
    RngStream stream;
    snprintf(descr, sizeof descr, "RngSream<%s>", MSG_host_get_name(host));
    stream = RngStream_CreateStream(descr);
    MSG_host_set_property_value(host, "stream", (char*)stream, NULL);
  }
}

static void chord_exit(void)
{
  msg_host_t host;
  unsigned i;
  xbt_dynar_foreach(host_list, i, host) {
    RngStream stream = (RngStream)MSG_host_get_property_value(host, "stream");
    RngStream_DeleteStream(&stream);
  }
  xbt_dynar_free(&host_list);

  xbt_free(powers2);
}

/**
 * \brief Turns an id into an equivalent id in [0, nb_keys).
 * \param id an id
 * \return the corresponding normalized id
 */
static int normalize(int id)
{
  // like id % nb_keys, but works with negatives numbers (and faster)
  return id & (nb_keys - 1);
}

/**
 * \brief Returns whether an id belongs to the interval [start, end].
 *
 * The parameters are noramlized to make sure they are between 0 and nb_keys - 1).
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
    end += nb_keys;
  }

  if (id < start) {
    id += nb_keys;
  }

  return id <= end;
}

/**
 * \brief Gets the mailbox name of a host given its chord id.
 * \param node_id id of a node
 * \param mailbox pointer to where the mailbox name should be written
 * (there must be enough space)
 */
static void get_mailbox(int node_id, char* mailbox)
{
  snprintf(mailbox, MAILBOX_NAME_SIZE - 1, "%d", node_id);
}

/**
 * \brief Frees the memory used by a task.
 * \param task the MSG task to destroy
 */
static void task_free(void* task)
{
  // TODO add a parameter data_free_function to MSG_task_create?
  if(task != NULL){
    xbt_free(MSG_task_get_data(task));
    MSG_task_destroy(task);
  }
}

/**
 * \brief Displays the finger table of a node.
 * \param node a node
 */
static void print_finger_table(node_t node)
{
  if (XBT_LOG_ISENABLED(msg_chord, xbt_log_priority_verbose)) {
    int i;
    XBT_VERB("My finger table:");
    XBT_VERB("Start | Succ");
    for (i = 0; i < nb_bits; i++) {
      XBT_VERB(" %3d  | %3d", (node->id + powers2[i]) % nb_keys, node->fingers[i].id);
    }
    XBT_VERB("Predecessor: %d", node->pred_id);
  }
}

/**
 * \brief Sets a finger of the current node.
 * \param node the current node
 * \param finger_index index of the finger to set (0 to nb_bits - 1)
 * \param id the id to set for this finger
 */
static void set_finger(node_t node, int finger_index, int id)
{
  if (id != node->fingers[finger_index].id) {
    node->fingers[finger_index].id = id;
    get_mailbox(id, node->fingers[finger_index].mailbox);
    node->last_change_date = MSG_get_clock();
    XBT_DEBUG("My new finger #%d is %d", finger_index, id);
  }
}

/**
 * \brief Sets the predecessor of the current node.
 * \param node the current node
 * \param id the id to predecessor, or -1 to unset the predecessor
 */
static void set_predecessor(node_t node, int predecessor_id)
{
  if (predecessor_id != node->pred_id) {
    node->pred_id = predecessor_id;

    if (predecessor_id != -1) {
      get_mailbox(predecessor_id, node->pred_mailbox);
    }
    node->last_change_date = MSG_get_clock();

    XBT_DEBUG("My new predecessor is %d", predecessor_id);
  }
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

  /* Reduce the run size for the MC */
  if(MC_is_active() || MC_record_replay_is_active()){
    periodic_stabilize_delay = 8;
    periodic_fix_fingers_delay = 8;
    periodic_check_predecessor_delay = 8;
  }

  double init_time = MSG_get_clock();
  msg_task_t task_received = NULL;
  int i;
  int join_success = 0;
  double deadline;
  double next_stabilize_date = init_time + periodic_stabilize_delay;
  double next_fix_fingers_date = init_time + periodic_fix_fingers_delay;
  double next_check_predecessor_date = init_time + periodic_check_predecessor_delay;
  double next_lookup_date = init_time + periodic_lookup_delay;

  int listen = 0;
  int no_op = 0;
  int sub_protocol = 0;

  xbt_assert(argc == 3 || argc == 5, "Wrong number of arguments for this node");

  // initialize my node
  s_node_t node = {0};
  node.id = xbt_str_parse_int(argv[1],"Invalid ID: %s");
  node.stream = (RngStream)MSG_host_get_property_value(MSG_host_self(), "stream");
  get_mailbox(node.id, node.mailbox);
  node.next_finger_to_fix = 0;
  node.fingers = xbt_new0(s_finger_t, nb_bits);
  node.last_change_date = init_time;

  for (i = 0; i < nb_bits; i++) {
    node.fingers[i].id = -1;
    set_finger(&node, i, node.id);
  }

  if (argc == 3) { // first ring
    deadline = xbt_str_parse_double(argv[2],"Invalid deadline: %s");
    create(&node);
    join_success = 1;

  } else {
    int known_id = xbt_str_parse_int(argv[2],"Invalid root ID: %s");
    //double sleep_time = atof(argv[3]);
    deadline = xbt_str_parse_double(argv[4],"Invalid deadline: %s");

    /*
    // sleep before starting
    XBT_DEBUG("Let's sleep during %f", sleep_time);
    MSG_process_sleep(sleep_time);
    */
    XBT_DEBUG("Hey! Let's join the system.");

    join_success = join(&node, known_id);
  }

  if (join_success) {
    while (MSG_get_clock() < init_time + deadline
//      && MSG_get_clock() < node.last_change_date + 1000
        && MSG_get_clock() < max_simulation_time) {

      if (node.comm_receive == NULL) {
        task_received = NULL;
        node.comm_receive = MSG_task_irecv(&task_received, node.mailbox);
        // FIXME: do not make MSG_task_irecv() calls from several functions
      }

      //XBT_INFO("Node %d is ring member : %d", node.id, is_ring_member(known_id, node.id) != -1);

      if (!MSG_comm_test(node.comm_receive)) {

        // no task was received: make some periodic calls

        if(MC_is_active() || MC_record_replay_is_active()){
          if(MC_is_active() && !MC_visited_reduction() && no_op){
              MC_cut();
          }
          if(listen == 0 && (sub_protocol = MC_random(0, 4)) > 0){
            if(sub_protocol == 1)
              stabilize(&node);
            else if(sub_protocol == 2)
              fix_fingers(&node);
            else if(sub_protocol == 3)
              check_predecessor(&node);
            else
              random_lookup(&node);
            listen = 1;
          }else{
            MSG_process_sleep(sleep_delay);
            if(!MC_visited_reduction())
              no_op = 1;
          }
        }else{
          if (MSG_get_clock() >= next_stabilize_date) {
            stabilize(&node);
            next_stabilize_date = MSG_get_clock() + periodic_stabilize_delay;
          }else if (MSG_get_clock() >= next_fix_fingers_date) {
            fix_fingers(&node);
            next_fix_fingers_date = MSG_get_clock() + periodic_fix_fingers_delay;
          }else if (MSG_get_clock() >= next_check_predecessor_date) {
            check_predecessor(&node);
            next_check_predecessor_date = MSG_get_clock() + periodic_check_predecessor_delay;
          }else if (MSG_get_clock() >= next_lookup_date) {
            random_lookup(&node);
            next_lookup_date = MSG_get_clock() + periodic_lookup_delay;
          }else {
            // nothing to do: sleep for a while
            MSG_process_sleep(sleep_delay);
          }
        }

      } else {
        // a transfer has occurred

        msg_error_t status = MSG_comm_get_status(node.comm_receive);

        if (status != MSG_OK) {
          XBT_DEBUG("Failed to receive a task. Nevermind.");
          MSG_comm_destroy(node.comm_receive);
          node.comm_receive = NULL;
        }
        else {
          // the task was successfully received
          MSG_comm_destroy(node.comm_receive);
          node.comm_receive = NULL;
          handle_task(&node, task_received);
        }
      }
    }

    if (node.comm_receive) {
      /* handle last task if any */
      if (MSG_comm_wait(node.comm_receive, 0) == MSG_OK)
        task_free(task_received);
      MSG_comm_destroy(node.comm_receive);
      node.comm_receive = NULL;
    }

    // leave the ring
    leave(&node);
  }

  // stop the simulation
  xbt_free(node.fingers);
  return 0;
}

/**
 * \brief This function is called when the current node receives a task.
 * \param node the current node
 * \param task the task to handle (don't touch it then:
 * it will be destroyed, reused or forwarded)
 */
static void handle_task(node_t node, msg_task_t task) {

  XBT_DEBUG("Handling task %p", task);
  char mailbox[MAILBOX_NAME_SIZE];
  task_data_t task_data = (task_data_t) MSG_task_get_data(task);
  e_task_type_t type = task_data->type;

  switch (type) {

  case TASK_FIND_SUCCESSOR:
    XBT_DEBUG("Receiving a 'Find Successor' request from %s for id %d",
              task_data->issuer_host_name, task_data->request_id);
    // is my successor the successor?
    if (is_in_interval(task_data->request_id, node->id + 1, node->fingers[0].id)) {
      task_data->type = TASK_FIND_SUCCESSOR_ANSWER;
      task_data->answer_id = node->fingers[0].id;
      XBT_DEBUG("Sending back a 'Find Successor Answer' to %s (mailbox %s): the successor of %d is %d",
                task_data->issuer_host_name,
                task_data->answer_to,
                task_data->request_id, task_data->answer_id);
      MSG_task_dsend(task, task_data->answer_to, task_free);
    }
    else {
      // otherwise, forward the request to the closest preceding finger in my table
      int closest = closest_preceding_node(node, task_data->request_id);
      XBT_DEBUG("Forwarding the 'Find Successor' request for id %d to my closest preceding finger %d",
                task_data->request_id, closest);
      get_mailbox(closest, mailbox);
      MSG_task_dsend(task, mailbox, task_free);
    }
    break;

  case TASK_GET_PREDECESSOR:
    XBT_DEBUG("Receiving a 'Get Predecessor' request from %s", task_data->issuer_host_name);
    task_data->type = TASK_GET_PREDECESSOR_ANSWER;
    task_data->answer_id = node->pred_id;
    XBT_DEBUG("Sending back a 'Get Predecessor Answer' to %s via mailbox '%s': my predecessor is %d",
              task_data->issuer_host_name,
              task_data->answer_to, task_data->answer_id);
    MSG_task_dsend(task, task_data->answer_to, task_free);
    break;

  case TASK_NOTIFY:
    // someone is telling me that he may be my new predecessor
    XBT_DEBUG("Receiving a 'Notify' request from %s", task_data->issuer_host_name);
    notify(node, task_data->request_id);
    task_free(task);
    break;

  case TASK_PREDECESSOR_LEAVING:
    // my predecessor is about to quit
    XBT_DEBUG("Receiving a 'Predecessor Leaving' message from %s", task_data->issuer_host_name);
    // modify my predecessor
    set_predecessor(node, task_data->request_id);
    task_free(task);
    /*TODO :
      >> notify my new predecessor
      >> send a notify_predecessors !!
    */
    break;

  case TASK_SUCCESSOR_LEAVING:
    // my successor is about to quit
    XBT_DEBUG("Receiving a 'Successor Leaving' message from %s", task_data->issuer_host_name);
    // modify my successor FIXME : this should be implicit ?
    set_finger(node, 0, task_data->request_id);
    task_free(task);
    /* TODO
       >> notify my new successor
       >> update my table & predecessors table */
    break;

  case TASK_FIND_SUCCESSOR_ANSWER:
  case TASK_GET_PREDECESSOR_ANSWER:
  case TASK_PREDECESSOR_ALIVE_ANSWER:
    XBT_DEBUG("Ignoring unexpected task of type %d (%p)", (int)type, task);
    task_free(task);
    break;

  case TASK_PREDECESSOR_ALIVE:
    XBT_DEBUG("Receiving a 'Predecessor Alive' request from %s", task_data->issuer_host_name);
    task_data->type = TASK_PREDECESSOR_ALIVE_ANSWER;
    XBT_DEBUG("Sending back a 'Predecessor Alive Answer' to %s (mailbox %s)",
              task_data->issuer_host_name,
              task_data->answer_to);
    MSG_task_dsend(task, task_data->answer_to, task_free);
    break;

  default:
    THROW_IMPOSSIBLE;
  }
}

/**
 * \brief Initializes the current node as the first one of the system.
 * \param node the current node
 */
static void create(node_t node)
{
  XBT_DEBUG("Create a new Chord ring...");
  set_predecessor(node, -1); // -1 means that I have no predecessor
  print_finger_table(node);
}

/**
 * \brief Makes the current node join the ring, knowing the id of a node
 * already in the ring
 * \param node the current node
 * \param known_id id of a node already in the ring
 * \return 1 if the join operation succeeded, 0 otherwise
 */
static int join(node_t node, int known_id)
{
  XBT_INFO("Joining the ring with id %d, knowing node %d", node->id, known_id);
  set_predecessor(node, -1); // no predecessor (yet)

  /*
  int i;
  for (i = 0; i < nb_bits; i++) {
    set_finger(node, i, known_id);
  }
  */

  int successor_id = remote_find_successor(node, known_id, node->id);
  if (successor_id == -1) {
    XBT_INFO("Cannot join the ring.");
  }
  else {
    set_finger(node, 0, successor_id);
    print_finger_table(node);
  }

  return successor_id != -1;
}

/**
 * \brief Makes the current node quit the system
 * \param node the current node
 */
static void leave(node_t node)
{
  XBT_DEBUG("Well Guys! I Think it's time for me to quit ;)");
  quit_notify(node);
}

/**
 * \brief Notifies the successor and the predecessor of the current node
 * of the departure
 * \param node the current node
 */
static void quit_notify(node_t node)
{
  char mailbox[MAILBOX_NAME_SIZE];
  //send the PREDECESSOR_LEAVING to our successor
  task_data_t req_data = xbt_new0(s_task_data_t,1);
  req_data->type = TASK_PREDECESSOR_LEAVING;
  req_data->request_id = node->pred_id;
  get_mailbox(node->id, req_data->answer_to);
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  msg_task_t task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  XBT_DEBUG("Sending a 'PREDECESSOR_LEAVING' to my successor %d",node->fingers[0].id);
  if (MSG_task_send_with_timeout(task_sent, node->fingers[0].mailbox, timeout)==
      MSG_TIMEOUT) {
    XBT_DEBUG("Timeout expired when sending a 'PREDECESSOR_LEAVING' to my successor %d",
        node->fingers[0].id);
    task_free(task_sent);
  }

  //send the SUCCESSOR_LEAVING to our predecessor
  get_mailbox(node->pred_id, mailbox);
  task_data_t req_data_s = xbt_new0(s_task_data_t,1);
  req_data_s->type = TASK_SUCCESSOR_LEAVING;
  req_data_s->request_id = node->fingers[0].id;
  req_data_s->request_id = node->pred_id;
  get_mailbox(node->id, req_data_s->answer_to);
  req_data_s->issuer_host_name = MSG_host_get_name(MSG_host_self());

  msg_task_t task_sent_s = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data_s);
  XBT_DEBUG("Sending a 'SUCCESSOR_LEAVING' to my predecessor %d",node->pred_id);
  if (MSG_task_send_with_timeout(task_sent_s, mailbox, timeout)==
      MSG_TIMEOUT) {
    XBT_DEBUG("Timeout expired when sending a 'SUCCESSOR_LEAVING' to my predecessor %d",
        node->pred_id);
    task_free(task_sent_s);
  }

}

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
  int stop = 0;
  char mailbox[MAILBOX_NAME_SIZE];
  get_mailbox(ask_to, mailbox);
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->type = TASK_FIND_SUCCESSOR;
  req_data->request_id = id;
  get_mailbox(node->id, req_data->answer_to);
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Find Successor" request to ask_to_id
  msg_task_t task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  XBT_DEBUG("Sending a 'Find Successor' request (task %p) to %d for id %d", task_sent, ask_to, id);
  msg_error_t res = MSG_task_send_with_timeout(task_sent, mailbox, timeout);

  if (res != MSG_OK) {
    XBT_DEBUG("Failed to send the 'Find Successor' request (task %p) to %d for id %d",
        task_sent, ask_to, id);
    task_free(task_sent);
  }
  else {

    // receive the answer
    XBT_DEBUG("Sent a 'Find Successor' request (task %p) to %d for key %d, waiting for the answer",
        task_sent, ask_to, id);

    do {
      if (node->comm_receive == NULL) {
        msg_task_t task_received = NULL;
        node->comm_receive = MSG_task_irecv(&task_received, node->mailbox);
      }

      res = MSG_comm_wait(node->comm_receive, timeout);

      if (res != MSG_OK) {
        XBT_DEBUG("Failed to receive the answer to my 'Find Successor' request (task %p): %d",
                  task_sent, (int)res);
        stop = 1;
        MSG_comm_destroy(node->comm_receive);
        node->comm_receive = NULL;
      }
      else {
        msg_task_t task_received = MSG_comm_get_task(node->comm_receive);
        XBT_DEBUG("Received a task (%p)", task_received);
        task_data_t ans_data = MSG_task_get_data(task_received);

  // Once upon a time, our code assumed that here, task_received != task_sent all the time
  //
  // This assumption is wrong (as messages from differing round can interleave), leading to a bug in our code.
  // We failed to find this bug directly, as it only occured on large platforms, leading to hardly usable traces.
  // Instead, we used the model-checker to track down the issue by adding the following test here in the code:
  //   if (MC_is_active()) {
  //      MC_assert(task_received == task_sent);
        //   }
  // That explained the bug in a snap, with a very cool example and everything.
  //
  // This MC_assert is now desactivated as the case is now properly handled in our code and we don't want the
  //   MC to fail any further under that condition, but this comment is here to as a memorial for this first
  //   brillant victory of the model-checking in the SimGrid community :)

        if (task_received != task_sent ||
            ans_data->type != TASK_FIND_SUCCESSOR_ANSWER) {
          // this is not the expected answer
          MSG_comm_destroy(node->comm_receive);
          node->comm_receive = NULL;
          handle_task(node, task_received);
        }
        else {
          // this is our answer
          XBT_DEBUG("Received the answer to my 'Find Successor' request for id %d (task %p): the successor of key %d is %d",
              ans_data->request_id, task_received, id, ans_data->answer_id);
          successor = ans_data->answer_id;
          stop = 1;
          MSG_comm_destroy(node->comm_receive);
          node->comm_receive = NULL;
          task_free(task_received);
        }
      }
    } while (!stop);
  }

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
  int stop = 0;
  char mailbox[MAILBOX_NAME_SIZE];
  get_mailbox(ask_to, mailbox);
  task_data_t req_data = xbt_new0(s_task_data_t, 1);
  req_data->type = TASK_GET_PREDECESSOR;
  get_mailbox(node->id, req_data->answer_to);
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  // send a "Get Predecessor" request to ask_to_id
  XBT_DEBUG("Sending a 'Get Predecessor' request to %d", ask_to);
  msg_task_t task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  msg_error_t res = MSG_task_send_with_timeout(task_sent, mailbox, timeout);

  if (res != MSG_OK) {
    XBT_DEBUG("Failed to send the 'Get Predecessor' request (task %p) to %d",
        task_sent, ask_to);
    task_free(task_sent);
  }
  else {

    // receive the answer
    XBT_DEBUG("Sent 'Get Predecessor' request (task %p) to %d, waiting for the answer on my mailbox '%s'",
        task_sent, ask_to, req_data->answer_to);

    do {
      if (node->comm_receive == NULL) { // FIXME simplify this
        msg_task_t task_received = NULL;
        node->comm_receive = MSG_task_irecv(&task_received, node->mailbox);
      }

      res = MSG_comm_wait(node->comm_receive, timeout);

      if (res != MSG_OK) {
        XBT_DEBUG("Failed to receive the answer to my 'Get Predecessor' request (task %p): %d",
            task_sent, (int)res);
        stop = 1;
        MSG_comm_destroy(node->comm_receive);
        node->comm_receive = NULL;
      }
      else {
        msg_task_t task_received = MSG_comm_get_task(node->comm_receive);
        task_data_t ans_data = MSG_task_get_data(task_received);

        /*if (MC_is_active()) {
          MC_assert(task_received == task_sent);
          }*/

        if (task_received != task_sent ||
            ans_data->type != TASK_GET_PREDECESSOR_ANSWER) {
          MSG_comm_destroy(node->comm_receive);
          node->comm_receive = NULL;
          handle_task(node, task_received);
        }
        else {
          XBT_DEBUG("Received the answer to my 'Get Predecessor' request (task %p): the predecessor of node %d is %d",
              task_received, ask_to, ans_data->answer_id);
          predecessor_id = ans_data->answer_id;
          stop = 1;
          MSG_comm_destroy(node->comm_receive);
          node->comm_receive = NULL;
          task_free(task_received);
        }
      }
    } while (!stop);
  }

  return predecessor_id;
}

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
  for (i = nb_bits - 1; i >= 0; i--) {
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
  XBT_DEBUG("Stabilizing node");

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
    XBT_DEBUG("I don't have to change my predecessor to %d", predecessor_candidate_id);
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
      req_data->type = TASK_NOTIFY;
      req_data->request_id = predecessor_candidate_id;
      req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

      // send a "Notify" request to notify_id
      msg_task_t task = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
      XBT_DEBUG("Sending a 'Notify' request (task %p) to %d", task, notify_id);
      char mailbox[MAILBOX_NAME_SIZE];
      get_mailbox(notify_id, mailbox);
      MSG_task_dsend(task, mailbox, task_free);
    }

/**
 * \brief This function is called periodically.
 * It refreshes the finger table of the current node.
 * \param node the current node
 */
static void fix_fingers(node_t node) {

  XBT_DEBUG("Fixing fingers");
  int i = node->next_finger_to_fix;
  int id = find_successor(node, node->id + powers2[i]);
  if (id != -1) {

    if (id != node->fingers[i].id) {
      set_finger(node, i, id);
      print_finger_table(node);
    }
    node->next_finger_to_fix = (i + 1) % nb_bits;
  }
}

/**
 * \brief This function is called periodically.
 * It checks whether the predecessor has failed
 * \param node the current node
 */
static void check_predecessor(node_t node)
{
  XBT_DEBUG("Checking whether my predecessor is alive");

  if(node->pred_id == -1)
    return;

  int stop = 0;

  char mailbox[MAILBOX_NAME_SIZE];
  get_mailbox(node->pred_id, mailbox);
  task_data_t req_data = xbt_new0(s_task_data_t,1);
  req_data->type = TASK_PREDECESSOR_ALIVE;
  req_data->request_id = node->pred_id;
  get_mailbox(node->id, req_data->answer_to);
  req_data->issuer_host_name = MSG_host_get_name(MSG_host_self());

  msg_task_t task_sent = MSG_task_create(NULL, COMP_SIZE, COMM_SIZE, req_data);
  XBT_DEBUG("Sending a 'Predecessor Alive' request to my predecessor %d", node->pred_id);

  msg_error_t res = MSG_task_send_with_timeout(task_sent, mailbox, timeout);

  if (res != MSG_OK) {
    XBT_DEBUG("Failed to send the 'Predecessor Alive' request (task %p) to %d", task_sent, node->pred_id);
    task_free(task_sent);
  }else{

    // receive the answer
    XBT_DEBUG("Sent 'Predecessor Alive' request (task %p) to %d, waiting for the answer on my mailbox '%s'",
              task_sent, node->pred_id, req_data->answer_to);

    do {
      if (node->comm_receive == NULL) { // FIXME simplify this
        msg_task_t task_received = NULL;
        node->comm_receive = MSG_task_irecv(&task_received, node->mailbox);
      }

      res = MSG_comm_wait(node->comm_receive, timeout);

      if (res != MSG_OK) {
        XBT_DEBUG("Failed to receive the answer to my 'Predecessor Alive' request (task %p): %d",
                  task_sent, (int)res);
        stop = 1;
        MSG_comm_destroy(node->comm_receive);
        node->comm_receive = NULL;
        node->pred_id = -1;
      }else {
        msg_task_t task_received = MSG_comm_get_task(node->comm_receive);
        if (task_received != task_sent) {
          MSG_comm_destroy(node->comm_receive);
          node->comm_receive = NULL;
          handle_task(node, task_received);
        }else{
          XBT_DEBUG("Received the answer to my 'Predecessor Alive' request (task %p) : my predecessor %d is alive",
                    task_received, node->pred_id);
          stop = 1;
          MSG_comm_destroy(node->comm_receive);
          node->comm_receive = NULL;
          task_free(task_received);
        }
      }
    } while (!stop);
  }
}

/**
 * \brief Performs a find successor request to a random id.
 * \param node the current node
 */
static void random_lookup(node_t node)
{
  int random_index = RngStream_RandInt (node->stream, 0, nb_bits - 1);
  int random_id = node->fingers[random_index].id;
  XBT_DEBUG("Making a lookup request for id %d", random_id);
  int res = find_successor(node, random_id);
  XBT_DEBUG("The successor of node %d is %d", random_id, res);

}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, 
       "Usage: %s [-nb_bits=n] [-timeout=t] platform_file deployment_file\n"
       "\tExample: %s ../msg_platform.xml chord.xml\n", argv[0], argv[0]);

  char **options = &argv[1];
  while (!strncmp(options[0], "-", 1)) {

    int length = strlen("-nb_bits=");
    if (!strncmp(options[0], "-nb_bits=", length) && strlen(options[0]) > length) {
      nb_bits = xbt_str_parse_int(options[0] + length, "Invalid nb_bits parameter: %s");
      XBT_DEBUG("Set nb_bits to %d", nb_bits);
    }
    else {

      length = strlen("-timeout=");
      if (!strncmp(options[0], "-timeout=", length) && strlen(options[0]) > length) {
        timeout = xbt_str_parse_int(options[0] + length, "Invalid timeout parameter: %s");
        XBT_DEBUG("Set timeout to %d", timeout);
      }
      else {
        xbt_die("Invalid chord option '%s'", options[0]);
      }
    }
    options++;
  }

  const char* platform_file = options[0];
  const char* application_file = options[1];

  MSG_create_environment(platform_file);

  chord_initialize();

  MSG_function_register("node", node);
  MSG_launch_application(application_file);

  msg_error_t res = MSG_main();
  XBT_CRITICAL("Messages created: %ld", smx_total_comms);
  XBT_INFO("Simulated time: %g", MSG_get_clock());

  chord_exit();

  return res != MSG_OK;
}
