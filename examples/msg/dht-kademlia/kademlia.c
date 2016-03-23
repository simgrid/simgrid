/* Copyright (c) 2012, 2014-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "kademlia.h"
#include "node.h"
#include "task.h"

#include "simgrid/msg.h"
/** @addtogroup MSG_examples
  * <b>kademlia/kademlia.c: Kademlia protocol</b>
  * Implements the Kademlia protocol, using 32 bits identifiers.
  */
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kademlia, "Messages specific for this msg example");

extern long unsigned int smx_total_comms;

/* Main loop for the process */
static void main_loop(node_t node, double deadline)
{
  double next_lookup_time = MSG_get_clock() + random_lookup_interval;
  XBT_VERB("Main loop start");
  while (MSG_get_clock() < deadline) {

    if (node->receive_comm == NULL) {
      node->task_received = NULL;
      node->receive_comm = MSG_task_irecv(&node->task_received, node->mailbox);
    }
    if (node->receive_comm) {
      if (!MSG_comm_test(node->receive_comm)) {
        /* We search for a pseudo random node */
        if (MSG_get_clock() >= next_lookup_time) {
          random_lookup(node);
          next_lookup_time += random_lookup_interval;
        } else {
          //Didn't get a task: sleep for a while...
          MSG_process_sleep(1);
        }
      } else {
        //There has been a transfer, we need to handle it !
        msg_error_t status = MSG_comm_get_status(node->receive_comm);
        MSG_comm_destroy(node->receive_comm);
        node->receive_comm = NULL;

        if (status == MSG_OK) {
          xbt_assert((node->task_received != NULL), "We received an incorrect task");
          handle_task(node, node->task_received);
        } else {
          xbt_assert((MSG_comm_get_task(node->receive_comm) == NULL), "Comm failed but received a task.");
          XBT_DEBUG("Nevermind, the communication has failed.");
        }
      }
    } else {
      //Didn't get a comm: sleep.
      MSG_process_sleep(1);
    }
  }
  //Cleanup the receiving communication.
  if (node->receive_comm != NULL) {
    if (MSG_comm_test(node->receive_comm) && MSG_comm_get_status(node->receive_comm) == MSG_OK) {
      task_free(MSG_comm_get_task(node->receive_comm));
    }
    MSG_comm_destroy(node->receive_comm);
  }
}

/** @brief Node function
  * @param my node ID
  * @param the ID of the person I know in the system (or not)
  * @param Time before I leave the system because I'm bored
  */
static int node(int argc, char *argv[])
{
  unsigned int join_sucess = 1;
  double deadline;
  xbt_assert(argc == 3 || argc == 4, "Wrong number of arguments");
  /* Node initialization */
  unsigned int id = strtoul(argv[1], NULL, 0);
  node_t node = node_init(id);

  if (argc == 4) {
    XBT_INFO("Hi, I'm going to join the network with id %s", node->mailbox);
    unsigned int id_known = strtoul(argv[2], NULL, 0);
    join_sucess = join(node, id_known);
    deadline = strtod(argv[3], NULL);
  } else {
    deadline = strtod(argv[2], NULL);
    XBT_INFO("Hi, I'm going to create the network with id %s", node->mailbox);
    node_routing_table_update(node, node->id);
  }
  if (join_sucess) {
    XBT_VERB("Ok, I'm joining the network with id %s", node->mailbox);
    //We start the main loop
    main_loop(node, deadline);
  } else {
    XBT_INFO("I couldn't join the network :(");
  }
  XBT_DEBUG("I'm leaving the network");
  XBT_INFO("%d/%d FIND_NODE have succeeded", node->find_node_success, node->find_node_success + node->find_node_failed);
  node_free(node);

  return 0;
}

/**
  * @brief Tries to join the network
  * @param node node data
  * @param id_known id of the node I know in the network.
  */
unsigned int join(node_t node, unsigned int id_known)
{
  answer_t node_list;
  msg_error_t status;
  unsigned int trial = 0;
  unsigned int i, answer_got = 0;

  /* Add the guy we know to our routing table and ourselves. */
  node_routing_table_update(node, node->id);
  node_routing_table_update(node, id_known);

  /* First step: Send a "FIND_NODE" request to the node we know */
  send_find_node(node, id_known, node->id);
  do {
    if (node->receive_comm == NULL) {
      node->task_received = NULL;
      node->receive_comm = MSG_task_irecv(&node->task_received, node->mailbox);
    }
    if (node->receive_comm) {
      if (MSG_comm_test(node->receive_comm)) {
        status = MSG_comm_get_status(node->receive_comm);
        MSG_comm_destroy(node->receive_comm);
        node->receive_comm = NULL;
        if (status == MSG_OK) {
          XBT_DEBUG("Received an answer from the node I know.");
          answer_got = 1;
          //retrieve the node list and ping them.
          task_data_t data = MSG_task_get_data(node->task_received);
          xbt_assert((data != NULL), "Null data received");
          if (data->type == TASK_FIND_NODE_ANSWER) {
            node_contact_t contact;
            node_list = data->answer;
            xbt_dynar_foreach(node_list->nodes, i, contact) {
              node_routing_table_update(node, contact->id);
            }
            task_free(node->task_received);
          } else {
            handle_task(node, node->task_received);
          }
        } else {
          trial++;
        }
      } else {
        MSG_process_sleep(1);
      }
    } else {
      MSG_process_sleep(1);
    }
  } while (answer_got == 0 && trial < max_join_trials);
  /* Second step: Send a FIND_NODE to a a random node in buckets */
  unsigned int bucket_id = routing_table_find_bucket(node->table, id_known)->id;
  for (i = 0; ((bucket_id - i) > 0 || (bucket_id + i) <= identifier_size) && i < JOIN_BUCKETS_QUERIES; i++) {
    if (bucket_id - i > 0) {
      unsigned int id_in_bucket = get_id_in_prefix(node->id, bucket_id - i);
      find_node(node, id_in_bucket, 0);
    }
    if (bucket_id + i <= identifier_size) {
      unsigned int id_in_bucket = get_id_in_prefix(node->id, bucket_id + i);
      find_node(node, id_in_bucket, 0);
    }
  }
  return answer_got;
}

/** @brief Send a request to find a node in the node routing table.
  * @param node our node data
  * @param id_to_find the id of the node we are trying to find
  */
unsigned int find_node(node_t node, unsigned int id_to_find, unsigned int count_in_stats)
{
  unsigned int i = 0;
  unsigned int queries, answers;
  unsigned int destination_found = 0;
  unsigned int nodes_added = 0;
  double time_beginreceive;
  double timeout, global_timeout = MSG_get_clock() + find_node_global_timeout;
  unsigned int steps = 0;

  xbt_assert((id_to_find >= 0), "Id supplied incorrect");

  /* First we build a list of who we already know */
  answer_t node_list = node_find_closest(node, id_to_find);
  xbt_assert((node_list != NULL), "node_list incorrect");

  XBT_DEBUG("Doing a FIND_NODE on %08x", id_to_find);

  msg_error_t status;

  /* Ask the nodes on our list if they   have information about the node we are trying to find */
  do {
    answers = 0;
    queries = send_find_node_to_best(node, node_list);
    nodes_added = 0;
    timeout = MSG_get_clock() + find_node_timeout;
    steps++;
    time_beginreceive = MSG_get_clock();
    do {
      if (node->receive_comm == NULL) {
        node->task_received = NULL;
        node->receive_comm = MSG_task_irecv(&node->task_received, node->mailbox);
      }
      if (node->receive_comm) {
        if (MSG_comm_test(node->receive_comm)) {
          status = MSG_comm_get_status(node->receive_comm);
          MSG_comm_destroy(node->receive_comm);
          node->receive_comm = NULL;
          if (status == MSG_OK) {
            xbt_assert((node->task_received != NULL), "Invalid task received");
            //Figure out if we received an answer or something else
            task_data_t data = MSG_task_get_data(node->task_received);
            xbt_assert((data != NULL), "No data in the task");

            //Check if what we have received is what we are looking for.
            if (data->type == TASK_FIND_NODE_ANSWER && data->answer->destination_id == id_to_find) {
              //Handle the answer
              node_routing_table_update(node, data->sender_id);
              node_contact_t contact;
              xbt_dynar_foreach(node_list->nodes, i, contact) {
                node_routing_table_update(node, contact->id);
              }
              answers++;

              nodes_added = answer_merge(node_list, data->answer);
              XBT_DEBUG("Received an answer from %s (%s) with %ld nodes on it",
                        data->answer_to, data->issuer_host_name, xbt_dynar_length(data->answer->nodes));

              task_free(node->task_received);
            } else {
              handle_task(node, node->task_received);
              //Update the timeout if we didn't have our answer
              timeout += MSG_get_clock() - time_beginreceive;
              time_beginreceive = MSG_get_clock();
            }
          }
        } else {
          MSG_process_sleep(1);
        }
      } else {
        MSG_process_sleep(1);
      }
    } while (MSG_get_clock() < timeout && answers < queries);
    destination_found = answer_destination_found(node_list);
  } while (!destination_found && (nodes_added > 0 || answers == 0) && MSG_get_clock() < global_timeout
            && steps < MAX_STEPS);
  if (destination_found) {
    if (count_in_stats)
      node->find_node_success++;
    if (queries > 4)
      XBT_VERB("FIND_NODE on %08x success in %d steps", id_to_find, steps);
    node_routing_table_update(node, id_to_find);
  } else {
    if (count_in_stats) {
      node->find_node_failed++;
      XBT_VERB("%08x not found in %d steps", id_to_find, steps);
    }
  }
  answer_free(node_list);
  return destination_found;
}

/** @brief Pings a node in the system to see if it is online.
  * @param node Our node data
  * @param id_to_ping the id of a node we want to see if it is online.
  * @return if the ping succeded or not.
  */
unsigned int ping(node_t node, unsigned int id_to_ping)
{
  char mailbox[MAILBOX_NAME_SIZE + 1];
  sprintf(mailbox, "%0*x", MAILBOX_NAME_SIZE, id_to_ping);

  unsigned int destination_found = 0;
  double timeout = MSG_get_clock() + ping_timeout;

  msg_task_t ping_task = task_new_ping(node->id, node->mailbox, MSG_host_get_name(MSG_host_self()));
  msg_task_t task_received = NULL;

  XBT_VERB("PING %08x", id_to_ping);

  //Check that we aren't trying to ping ourselves
  if (id_to_ping == node->id) {
    return 1;
  }

  /* Sending the ping task */
  MSG_task_dsend(ping_task, mailbox, task_free_v);
  do {
    task_received = NULL;
    msg_error_t status =
        MSG_task_receive_with_timeout(&task_received, node->mailbox, ping_timeout);
    if (status == MSG_OK) {
      xbt_assert((task_received != NULL), "Invalid task received");
      //Checking if it's what we are waiting for or not.
      task_data_t data = MSG_task_get_data(task_received);
      xbt_assert((data != NULL), "didn't receive any data...");
      if (data->type == TASK_PING_ANSWER && id_to_ping == data->sender_id) {
        XBT_VERB("Ping to %s succeeded", mailbox);
        node_routing_table_update(node, data->sender_id);
        destination_found = 1;
        task_free(task_received);
      } else {
        //If it's not our answer, we answer the query anyway.
        handle_task(node, task_received);
      }
    }
  } while (destination_found == 0 && MSG_get_clock() < timeout);

  if (MSG_get_clock() >= timeout) {
    XBT_DEBUG("Ping to %s has timeout.", mailbox);
    return 0;
  }
  if (destination_found == -1) {
    XBT_DEBUG("It seems that %s is offline...", mailbox);
    return 0;
  }
  return 1;
}

/** @brief Does a pseudo-random lookup for someone in the system
  * @param node caller node data
  */
void random_lookup(node_t node)
{
  unsigned int id_to_look = RANDOM_LOOKUP_NODE; //Totally random.
  /* TODO: Use some pseudorandom generator like RngStream. */
  XBT_DEBUG("I'm doing a random lookup");
  find_node(node, id_to_look, 1);
}

/** @brief Send a "FIND_NODE" to a node
  * @param node sender node data
  * @param id node we are querying
  * @param destination node we are trying to find.
  */
void send_find_node(node_t node, unsigned int id, unsigned int destination)
{
  char mailbox[MAILBOX_NAME_SIZE + 1];
  /* Gets the mailbox to send to */
  get_node_mailbox(id, mailbox);
  /* Build the task */
  msg_task_t task = task_new_find_node(node->id, destination, node->mailbox, MSG_host_get_name(MSG_host_self()));
  /* Send the task */
  xbt_assert((task != NULL), "Trying to send a NULL task.");
  MSG_task_dsend(task, mailbox, task_free_v);
  XBT_VERB("Asking %s for its closest nodes", mailbox);
}

/**
  * Sends to the best "kademlia_alpha" nodes in the "node_list" array a "FIND_NODE" request, to ask them for their best nodes
  */
unsigned int send_find_node_to_best(node_t node, answer_t node_list)
{
  unsigned int i = 0, j = 0;
  unsigned int destination = node_list->destination_id;
  node_contact_t node_to_query;
  while (j < kademlia_alpha && i < node_list->size) {
    /* We need to have at most "kademlia_alpha" requests each time, according to the protocol */
    /* Gets the node we want to send the query to */
    node_to_query = xbt_dynar_get_as(node_list->nodes, i, node_contact_t);
    if (node_to_query->id != node->id) {        /* No need to query ourselves */
      send_find_node(node, node_to_query->id, destination);
      j++;
    }
    i++;
  }
  return i;
}

/** @brief Handles an incoming received task */
void handle_task(node_t node, msg_task_t task)
{
  task_data_t data = MSG_task_get_data(task);
  xbt_assert((data != NULL), "Received NULL data");
  //Adding/updating the guy to our routing table
  node_routing_table_update(node, data->sender_id);
  switch (data->type) {
  case TASK_FIND_NODE:
    handle_find_node(node, data);
    break;
  case TASK_FIND_NODE_ANSWER:
    XBT_DEBUG("Received a wrong answer for a FIND_NODE");
    break;
  case TASK_PING:
    handle_ping(node, data);
    break;
  default:
    break;
  }
  task_free(task);
}

/** @brief Handles the answer to an incoming "find_node" task */
void handle_find_node(node_t node, task_data_t data)
{
  XBT_VERB("Received a FIND_NODE from %s (%s), he's trying to find %08x",
           data->answer_to, data->issuer_host_name, data->destination_id);
  //Building the answer to the request
  answer_t answer = node_find_closest(node, data->destination_id);
  //Building the task to send
  msg_task_t task = task_new_find_node_answer(node->id, data->destination_id, answer, node->mailbox,
                                              MSG_host_get_name(MSG_host_self()));
  //Sending the task
  MSG_task_dsend(task, data->answer_to, task_free_v);
}

/** @brief handles the answer to a ping */
void handle_ping(node_t node, task_data_t data)
{
  XBT_VERB("Received a PING request from %s (%s)", data->answer_to, data->issuer_host_name);
  //Building the answer to the request
  msg_task_t task = task_new_ping_answer(node->id, data->answer_to, MSG_host_get_name(MSG_host_self()));

  MSG_task_dsend(task, data->answer_to, task_free_v);
}

/** @brief Main function */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  const char *platform_file = argv[1];
  const char *deployment_file = argv[2];

  MSG_create_environment(platform_file);
  MSG_function_register("node", node);
  MSG_launch_application(deployment_file);

  msg_error_t res = MSG_main();

  XBT_CRITICAL("Messages created: %ld", smx_total_comms);
  XBT_INFO("Simulated time: %g", MSG_get_clock());

  return res != MSG_OK;
}
