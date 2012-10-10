/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * Copyright (c) 2012. Maximiliano Geier.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

/** @addtogroup MSG_examples
 * 
 *  - <b>kadeploy/kadeploy.c: Kadeploy implementation</b>.
 */


XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kadeploy,
                             "Messages specific for kadeploy");

#define MESSAGE_SIZE 1
#define PIECE_COUNT 1000
#define HOSTNAME_LENGTH 20

#define PEER_SHUTDOWN_DEADLINE 6000

/*
 Data structures
 */

/* Random iterator for xbt_dynar */
typedef struct xbt_dynar_iterator_struct {
  xbt_dynar_t list;
  xbt_dynar_t indices_list;
  int current;
  unsigned long length;
  xbt_dynar_t (*criteria_fn)(int size);
} *xbt_dynar_iterator_t;
typedef struct xbt_dynar_iterator_struct xbt_dynar_iterator_s;

/* Messages enum */
typedef enum {
  MESSAGE_BUILD_CHAIN = 0,
  MESSAGE_SEND_DATA,
  MESSAGE_END_DATA
} e_message_type;

/* Message struct */
typedef struct s_message {
  e_message_type type;
  const char *issuer_hostname;
  const char *mailbox;
  const char *prev_hostname;
  const char *next_hostname;
  const char *data_block;
  unsigned int data_length;
} s_message_t, *message_t;

/* Peer struct */
typedef struct s_peer {
  int init;
  const char *prev;
  const char *next;
  const char *me;
  int pieces;
  xbt_dynar_t pending_sends;
  int close_asap; /* TODO: unused */
} s_peer_t, *peer_t;

/* Iterator methods */
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, xbt_dynar_t (*criteria_fn)(int));
void *xbt_dynar_iterator_next(xbt_dynar_iterator_t it);
void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it);

/* Iterator generators */
xbt_dynar_t forward_indices_list(int size);
xbt_dynar_t reverse_indices_list(int size);
xbt_dynar_t random_indices_list(int size);

/* Message methods */
msg_task_t task_message_new(e_message_type type, const char *issuer_hostname, const char *mailbox);
msg_task_t task_message_chain_new(const char *issuer_hostname, const char *mailbox, const char* prev, const char *next);
msg_task_t task_message_data_new(const char *issuer_hostname, const char *mailbox, const char *block, unsigned int len);
msg_task_t task_message_end_data_new(const char *issuer_hostname, const char *mailbox);
void task_message_delete(void *);

/* Tasks */
int broadcaster(int argc, char *argv[]);
int peer(int argc, char *argv[]);

xbt_dynar_t build_hostlist_from_hostcount(int hostcount); 
/*xbt_dynar_t build_hostlist_from_argv(int argc, char *argv[]);*/

/* Broadcaster: helper functions */
int broadcaster_build_chain(const char **first, xbt_dynar_t host_list);
int broadcaster_send_file(const char *first);
int broadcaster_finish(xbt_dynar_t host_list);

/* Peer: helper functions */
msg_error_t peer_wait_for_message(peer_t peer);
int peer_execute_task(peer_t peer, msg_task_t task);
void peer_init_chain(peer_t peer, message_t msg);
void peer_shutdown(peer_t p);
void peer_init(peer_t p);

/* Initialization stuff */
msg_error_t test_all(const char *platform_file,
                     const char *application_file);

/* Shuffle */
/**************************************/
static int rand_int(int n);
void xbt_dynar_shuffle_in_place(xbt_dynar_t indices_list);

#define xbt_dynar_swap_elements(d, type, i, j) \
  type tmp; \
  tmp = xbt_dynar_get_as(indices_list, (unsigned int)j, type); \
  xbt_dynar_set_as(indices_list, (unsigned int)j, type, \
    xbt_dynar_get_as(indices_list, (unsigned int)i, type)); \
  xbt_dynar_set_as(indices_list, (unsigned int)i, type, tmp);

/* http://stackoverflow.com/a/3348142 */
static int rand_int(int n)
{
  int limit = RAND_MAX - RAND_MAX % n;
  int rnd;

  do {
    rnd = rand();
  } while (rnd >= limit);
  
  return rnd % n;
}

void xbt_dynar_shuffle_in_place(xbt_dynar_t indices_list)
{
  int i, j;

  for (i = xbt_dynar_length(indices_list) - 1; i > 0; i--) {
    j = rand_int(i + 1);
    xbt_dynar_swap_elements(indices_list, int, i, j);
  }
}
/**************************************/

/* Allocates and initializes a new xbt_dynar iterator for list, using criteria_fn as iteration criteria
   criteria_fn: given an array size, it must generate a list containing the indices of every item in some order */
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, xbt_dynar_t (*criteria_fn)(int))
{
  xbt_dynar_iterator_t it = xbt_new(xbt_dynar_iterator_s, 1);
  
  it->list = list;
  it->length = xbt_dynar_length(list);
  it->indices_list = criteria_fn(it->length); //xbt_dynar_new(sizeof(int), NULL);
  it->criteria_fn = criteria_fn;
  it->current = 0;
}

void xbt_dynar_iterator_reset(xbt_dynar_iterator_t it)
{
  xbt_dynar_free_container(&(it->indices_list));
  it->indices_list = it->criteria_fn(it->length);
  it->current = 0;
}

void xbt_dynar_iterator_seek(xbt_dynar_iterator_t it, int pos)
{
  it->current = pos;
}

/* Returns the next element iterated by iterator it, NULL if there are no more elements */
void *xbt_dynar_iterator_next(xbt_dynar_iterator_t it)
{
  int *next;
  //XBT_INFO("%d current\n", next);
  if (it->current >= it->length) {
    //XBT_INFO("Nothing to return!\n");
    return NULL;
  } else {
    next = xbt_dynar_get_ptr(it->indices_list, it->current);
    it->current++;
    return xbt_dynar_get_ptr(it->list, *next);
  }
}

void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it)
{
  xbt_dynar_free_container(&(it->indices_list));
  xbt_free_ref(&it);
}

xbt_dynar_t forward_indices_list(int size)
{
  xbt_dynar_t indices_list = xbt_dynar_new(sizeof(int), NULL);
  int i;
  for (i = 0; i < size; i++)
    xbt_dynar_push_as(indices_list, int, i);
  return indices_list;
}

xbt_dynar_t reverse_indices_list(int size)
{
  xbt_dynar_t indices_list = xbt_dynar_new(sizeof(int), NULL);
  int i;
  for (i = size-1; i >= 0; i--)
    xbt_dynar_push_as(indices_list, int, i);
  return indices_list;
}

xbt_dynar_t random_indices_list(int size)
{
  xbt_dynar_t indices_list = forward_indices_list(size);
  xbt_dynar_shuffle_in_place(indices_list);
  return indices_list;
}


msg_task_t task_message_new(e_message_type type, const char *issuer_hostname, const char *mailbox)
{
  message_t msg = xbt_new(s_message_t, 1);
  msg->type = type;
  msg->issuer_hostname = issuer_hostname;
  msg->mailbox = mailbox;
  msg_task_t task = MSG_task_create(NULL, 0, MESSAGE_SIZE, msg); 

  return task;
}

msg_task_t task_message_chain_new(const char *issuer_hostname, const char *mailbox, const char* prev, const char *next)
{
  msg_task_t task = task_message_new(MESSAGE_BUILD_CHAIN, issuer_hostname, mailbox);
  message_t msg = MSG_task_get_data(task);
  msg->prev_hostname = prev;
  msg->next_hostname = next;

  return task;
}

msg_task_t task_message_data_new(const char *issuer_hostname, const char *mailbox, const char *block, unsigned int len)
{
  msg_task_t task = task_message_new(MESSAGE_SEND_DATA, issuer_hostname, mailbox);
  if (strcmp(mailbox, "host4") == 0) MSG_task_set_category(task, mailbox);
  message_t msg = MSG_task_get_data(task);
  msg->data_block = block;
  msg->data_length = len;

  return task;
}

msg_task_t task_message_end_data_new(const char *issuer_hostname, const char *mailbox)
{
  return task_message_new(MESSAGE_END_DATA, issuer_hostname, mailbox);
}

void task_message_delete(void *task)
{
  message_t msg = MSG_task_get_data(task);
  xbt_free(msg);
  MSG_task_destroy(task);
}

inline void queue_pending_connection(msg_comm_t comm, xbt_dynar_t q)
{
  xbt_dynar_push(q, &comm);
}

int process_pending_connections(xbt_dynar_t q)
{
  unsigned int iter;
  int status;
  int empty = 0;
  msg_comm_t comm;

  xbt_dynar_foreach(q, iter, comm) {
    empty = 1;
    if (MSG_comm_test(comm)) {
      MSG_comm_destroy(comm);
      status = MSG_comm_get_status(comm);
      xbt_assert(status == MSG_OK, __FILE__ ": process_pending_connections() failed");
      xbt_dynar_cursor_rm(q, &iter);
      empty = 0;
    }
  }
  return empty;
}

xbt_dynar_t build_hostlist_from_hostcount(int hostcount)
{
  xbt_dynar_t host_list = xbt_dynar_new(sizeof(char*), NULL);
  char *hostname = NULL;
  msg_host_t h = NULL;
  int i = 1;
  
  for (; i < hostcount+1; i++) {
    hostname = xbt_new(char, HOSTNAME_LENGTH);
    snprintf(hostname, HOSTNAME_LENGTH, "host%d", i);
    //XBT_INFO("%s", hostname);
    h = MSG_get_host_by_name(hostname);
    if (h == NULL) {
      XBT_INFO("Unknown host %s. Stopping Now! ", hostname);
      abort();
    } else {
      xbt_dynar_push(host_list, &hostname);
    }
  }
  return host_list;
}

/*xbt_dynar_t build_hostlist_from_argv(int argc, char *argv[])
{
  xbt_dynar_t host_list = xbt_dynar_new(sizeof(char*), NULL);
  msg_host_t h = NULL;
  int i = 1;
  
  for (; i < argc; i++) {
    XBT_INFO("host%d = %s", i, argv[i]);
    h = MSG_get_host_by_name(argv[i]);
    if (h == NULL) {
      XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
      abort();
    } else {
      xbt_dynar_push(host_list, &(argv[i]));
    }
  }
  return host_list;
}*/

void delete_hostlist(xbt_dynar_t h)
{
  xbt_dynar_free(&h);
}

int broadcaster_build_chain(const char **first, xbt_dynar_t host_list)
{
  xbt_dynar_iterator_t it = xbt_dynar_iterator_new(host_list, forward_indices_list);
  msg_task_t task = NULL;
  char **cur = (char**)xbt_dynar_iterator_next(it);
  const char *me = MSG_host_get_name(MSG_host_self());
  const char *current_host = NULL;
  const char *prev = NULL;
  const char *next = NULL;
  const char *last = NULL;

  /* Build the chain if there's at least one peer */
  if (cur != NULL) {
    /* init: prev=NULL, host=current cur, next=next cur */
    next = *cur;
    *first = next;

    /* This iterator iterates one step ahead: cur is current iterated element, 
       but it's actually the next one in the chain */
    do {
      /* following steps: prev=last, host=next, next=cur */
      cur = (char**)xbt_dynar_iterator_next(it);
      prev = last;
      current_host = next;
      if (cur != NULL)
        next = *cur;
      else
        next = NULL;
      //XBT_INFO("Building chain -- broadcaster:\"%s\" dest:\"%s\" prev:\"%s\" next:\"%s\"", me, current_host, prev, next);
    
      /* Send message to current peer */
      task = task_message_chain_new(me, current_host, prev, next);
      //MSG_task_set_category(task, current_host);
      MSG_task_send(task, current_host);

      last = current_host;
    } while (cur != NULL);
  }
  xbt_dynar_iterator_delete(it);

  return MSG_OK;
}

int broadcaster_send_file(const char *first)
{
  const char *me = MSG_host_get_name(MSG_host_self());
  msg_task_t task = NULL;
  msg_comm_t comm = NULL;
  int status;

  int piece_count = PIECE_COUNT;
  int cur = 0;

  for (; cur < piece_count; cur++) {
    task = task_message_data_new(me, first, NULL, 0);
    XBT_INFO("Sending (send) from %s into mailbox %s", me, first);
    status = MSG_task_send(task, first);
   
    xbt_assert(status == MSG_OK, __FILE__ ": broadcaster_send_file() failed");
  }

  return MSG_OK;
}

/* FIXME: I should iterate nodes in the same order as the one used to build the chain */
int broadcaster_finish(xbt_dynar_t host_list)
{
  xbt_dynar_iterator_t it = xbt_dynar_iterator_new(host_list, forward_indices_list);
  msg_task_t task = NULL;
  const char *me = MSG_host_get_name(MSG_host_self());
  const char *current_host = NULL;
  char **cur = NULL;

  /* Send goodbye message to every peer */
  for (cur = (char**)xbt_dynar_iterator_next(it); cur != NULL; cur = (char**)xbt_dynar_iterator_next(it)) {
    /* Send message to current peer */
    current_host = *cur;
    task = task_message_end_data_new(me, current_host);
    //MSG_task_set_category(task, current_host);
    MSG_task_send(task, current_host);
  }

  return MSG_OK;
}


/** Emitter function  */
int broadcaster(int argc, char *argv[])
{
  xbt_dynar_t host_list = NULL;
  const char *first = NULL;
  int status = !MSG_OK;

  XBT_INFO("broadcaster");

  /* Check that every host given by the hostcount in argv[1] exists and add it
     to a dynamic array */
  host_list = build_hostlist_from_hostcount(atoi(argv[1]));
  /*host_list = build_hostlist_from_argv(argc, argv);*/
  
  /* TODO: Error checking */
  status = broadcaster_build_chain(&first, host_list);
  status = broadcaster_send_file(first);
  status = broadcaster_finish(host_list);

  delete_hostlist(host_list);

  return status;
}

/*******************************************************
 *                     Peer                            *
 *******************************************************/

void peer_init_chain(peer_t peer, message_t msg)
{
  peer->prev = msg->prev_hostname;
  peer->next = msg->next_hostname;
  peer->init = 1;
}

void peer_forward_msg(peer_t peer, message_t msg)
{
  int status;
  msg_task_t task = task_message_data_new(peer->me, peer->next, NULL, 0);
  msg_comm_t comm = NULL;
  XBT_INFO("Sending (isend) from %s into mailbox %s", peer->me, peer->next);
  comm = MSG_task_isend(task, peer->next);
  queue_pending_connection(comm, peer->pending_sends);
}

int peer_execute_task(peer_t peer, msg_task_t task)
{
  int done = 0;
  message_t msg = MSG_task_get_data(task);
  
  //XBT_INFO("Peer %s got message of type %d\n", peer->me, msg->type);
  switch (msg->type) {
    case MESSAGE_BUILD_CHAIN:
      peer_init_chain(peer, msg);
      break;
    case MESSAGE_SEND_DATA:
      xbt_assert(peer->init, __FILE__ ": peer_execute_task() failed: got msg_type %d before initialization", msg->type);
      if (peer->next != NULL)
        peer_forward_msg(peer, msg);
      peer->pieces++;
      break;
    case MESSAGE_END_DATA:
      xbt_assert(peer->init, __FILE__ ": peer_execute_task() failed: got msg_type %d before initialization", msg->type);
      done = 1;
      XBT_INFO("%d pieces receieved", peer->pieces);
      break;
  }

  MSG_task_execute(task);

  return done;
}

msg_error_t peer_wait_for_message(peer_t peer)
{
  msg_error_t status;
  msg_comm_t comm = NULL;
  msg_task_t task = NULL;
  int done = 0;

  while (!done) {
    if (comm == NULL)
      comm = MSG_task_irecv(&task, peer->me);

    if (MSG_comm_test(comm)) {
      status = MSG_comm_get_status(comm);
      //XBT_INFO("peer_wait_for_message: error code = %d", status);
      xbt_assert(status == MSG_OK, __FILE__ ": peer_wait_for_message() failed");
      MSG_comm_destroy(comm);
      comm = NULL;
      done = peer_execute_task(peer, task);
      task_message_delete(task);
      task = NULL;
    } else {
      process_pending_connections(peer->pending_sends);
      MSG_process_sleep(0.1);
    }
  }

  return status;
}

void peer_init(peer_t p)
{
  p->init = 0;
  p->prev = NULL;
  p->next = NULL;
  p->pieces = 0;
  p->close_asap = 0;
  p->pending_sends = xbt_dynar_new(sizeof(msg_comm_t), NULL);
  p->me = MSG_host_get_name(MSG_host_self());
}

void peer_shutdown(peer_t p)
{
  float start_time = MSG_get_clock();
  float end_time = start_time + PEER_SHUTDOWN_DEADLINE;

  XBT_INFO("Waiting for sends to finish before shutdown...");
  while (xbt_dynar_length(p->pending_sends) && MSG_get_clock() < end_time) {
    process_pending_connections(p->pending_sends);
    MSG_process_sleep(0.1);
  }

  xbt_assert(xbt_dynar_length(p->pending_sends) == 0, "Shutdown failed, sends still pending after deadline");
  xbt_dynar_free(&p->pending_sends);

  xbt_free(p);
}

/** Peer function  */
int peer(int argc, char *argv[])
{
  peer_t p = xbt_new(s_peer_t, 1);
  msg_error_t status;

  XBT_INFO("peer");

  peer_init(p);
  status = peer_wait_for_message(p);
  peer_shutdown(p);

  return MSG_OK;
}                               /* end_of_receiver */


/** Test function */
msg_error_t test_all(const char *platform_file,
                     const char *application_file)
{

  msg_error_t res = MSG_OK;



  XBT_INFO("test_all");

  /*  Simulation setting */
  MSG_create_environment(platform_file);

  /* Trace categories */
  TRACE_category_with_color("host0", "0 0 1");
  TRACE_category_with_color("host1", "0 1 0");
  TRACE_category_with_color("host2", "0 1 1");
  TRACE_category_with_color("host3", "1 0 0");
  TRACE_category_with_color("host4", "1 0 1");
  TRACE_category_with_color("host5", "1 1 0");

  /*   Application deployment */
  MSG_function_register("broadcaster", broadcaster);
  MSG_function_register("peer", peer);

  MSG_launch_application(application_file);

  res = MSG_main();

  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

#ifdef _MSC_VER
  unsigned int prev_exponent_format =
      _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  MSG_init(&argc, argv);

  /*if (argc <= 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file <model>\n",
              argv[0]);
    XBT_CRITICAL
        ("example: %s msg_platform.xml msg_deployment.xml KCCFLN05_Vegas\n",
         argv[0]);
    exit(1);
  }*/

  /* Options for the workstation/model:

     KCCFLN05              => for maxmin
     KCCFLN05_proportional => for proportional (Vegas)
     KCCFLN05_Vegas        => for TCP Vegas
     KCCFLN05_Reno         => for TCP Reno
   */
  //MSG_config("workstation/model", argv[3]);

  res = test_all(argv[1], argv[2]);

  XBT_INFO("Total simulation time: %le", MSG_get_clock());

  MSG_clean();

#ifdef _MSC_VER
  _set_output_format(prev_exponent_format);
#endif

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
