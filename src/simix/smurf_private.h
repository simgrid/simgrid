/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SMURF_PRIVATE_H
#define _SIMIX_SMURF_PRIVATE_H

/********************************* Requests ***********************************/

/* we want to build the e_smx_t enumeration and the table of the corresponding
 * strings automatically, using macros */

#define SIMIX_REQ_LIST1 \
SIMIX_REQ_ENUM_ELEMENT(REQ_NO_REQ),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_BY_NAME),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_NAME),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_PROPERTIES),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_SPEED),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_AVAILABLE_SPEED),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_STATE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_GET_DATA),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_SET_DATA),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_PARALLEL_EXECUTE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTION_DESTROY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTION_CANCEL),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTION_GET_REMAINS),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTION_GET_STATE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTION_SET_PRIORITY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_HOST_EXECUTION_WAIT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_CREATE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_KILL),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_KILLALL),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_CLEANUP),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_CHANGE_HOST),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_SUSPEND),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_RESUME),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_COUNT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_GET_DATA),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_SET_DATA),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_GET_HOST),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_GET_NAME),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_IS_SUSPENDED),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_GET_PROPERTIES),\
SIMIX_REQ_ENUM_ELEMENT(REQ_PROCESS_SLEEP),\
SIMIX_REQ_ENUM_ELEMENT(REQ_RDV_CREATE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_RDV_DESTROY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_RDV_GEY_BY_NAME),\
SIMIX_REQ_ENUM_ELEMENT(REQ_RDV_COMM_COUNT_BY_HOST),\
SIMIX_REQ_ENUM_ELEMENT(REQ_RDV_GET_HEAD),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_SEND),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_ISEND),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_RECV),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_IRECV),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_DESTROY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_CANCEL),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_WAITANY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_WAIT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_TEST),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_TESTANY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_GET_REMAINS),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_GET_STATE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_GET_SRC_DATA),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_GET_DST_DATA),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_GET_SRC_PROC),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_GET_DST_PROC),\
SIMIX_REQ_ENUM_ELEMENT(REQ_MUTEX_INIT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_MUTEX_DESTROY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_MUTEX_LOCK),\
SIMIX_REQ_ENUM_ELEMENT(REQ_MUTEX_TRYLOCK),\
SIMIX_REQ_ENUM_ELEMENT(REQ_MUTEX_UNLOCK),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COND_INIT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COND_DESTROY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COND_SIGNAL),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COND_WAIT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COND_WAIT_TIMEOUT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_COND_BROADCAST),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_INIT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_DESTROY),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_RELEASE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_WOULD_BLOCK),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_ACQUIRE),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_ACQUIRE_TIMEOUT),\
SIMIX_REQ_ENUM_ELEMENT(REQ_SEM_GET_CAPACITY)

/* REQ_COMM_IS_LATENCY_BOUNDED and REQ_SET_CATEGORY make things complicated
 * because they are not always present */
#ifdef HAVE_LATENCY_BOUND_TRACKING
#define SIMIX_REQ_LIST2 \
,SIMIX_REQ_ENUM_ELEMENT(REQ_COMM_IS_LATENCY_BOUNDED)
#else
#define SIMIX_REQ_LIST2
#endif

#ifdef HAVE_TRACING
#define SIMIX_REQ_LIST3 \
,SIMIX_REQ_ENUM_ELEMENT(REQ_SET_CATEGORY)
#else
#define SIMIX_REQ_LIST3
#endif

/* SIMIX_REQ_LIST is the final macro to use */
#define SIMIX_REQ_LIST SIMIX_REQ_LIST1 SIMIX_REQ_LIST2 SIMIX_REQ_LIST3

/* you can redefine the following macro differently to generate something else
 * with the list of enumeration values (e.g. a table of strings or a table of function pointers) */
#define SIMIX_REQ_ENUM_ELEMENT(x) x

/**
 * \brief All possible SIMIX requests.
 */
typedef enum {
SIMIX_REQ_LIST
} e_smx_req_t;

/**
 * \brief Represents a SIMIX request.
 */
typedef struct s_smx_req {
  e_smx_req_t call;
  smx_process_t issuer;

  union {

    struct {
      const char *name;
      smx_host_t result;
    } host_get_by_name;

    struct {
      smx_host_t host;
      const char* result;
    } host_get_name;

    struct {
      smx_host_t host;
      xbt_dict_t result;
    } host_get_properties;

    struct {
      smx_host_t host;
      double result;
    } host_get_speed;

    struct {
      smx_host_t host;
      double result;
    } host_get_available_speed;

    struct {
      smx_host_t host;
      int result;
    } host_get_state;

    struct {
      smx_host_t host;
      void* result;
    } host_get_data;

    struct {
      smx_host_t host;
      void* data;
    } host_set_data;

    struct {
      const char* name;
      smx_host_t host;
      double computation_amount;
      double priority;
      smx_action_t result;
    } host_execute;

    struct {
      const char *name;
      int host_nb;
      smx_host_t *host_list;
      double *computation_amount;
      double *communication_amount;
      double amount;
      double rate;
      smx_action_t result;
    } host_parallel_execute;

    struct {
      smx_action_t execution;
    } host_execution_destroy;

    struct {
      smx_action_t execution;
    } host_execution_cancel;

    struct {
      smx_action_t execution;
      double result;
    } host_execution_get_remains;

    struct {
      smx_action_t execution;
      e_smx_state_t result;
    } host_execution_get_state;

    struct {
      smx_action_t execution;
      double priority;
    } host_execution_set_priority;

    struct {
      smx_action_t execution;
      e_smx_state_t result;
    } host_execution_wait;

    struct {
      smx_process_t *process;
      const char *name;
      xbt_main_func_t code;
      void *data;
      const char *hostname;
      int argc;
      char **argv;
      xbt_dict_t properties;
    } process_create;

    struct {
      smx_process_t process;
    } process_kill;

    struct {
      smx_process_t process;
    } process_cleanup;

    struct {
      smx_process_t process;
      smx_host_t dest;
    } process_change_host;

    struct {
      smx_process_t process;
    } process_suspend;

    struct {
      smx_process_t process;
    } process_resume;

    struct {
      int result;
    } process_count;

    struct {
      smx_process_t process;
      void* result;
    } process_get_data;

    struct {
      smx_process_t process;
      void* data;
    } process_set_data;

    struct {
      smx_process_t process;
      smx_host_t result;
    } process_get_host;

    struct {
      smx_process_t process;
      const char *result;
    } process_get_name;

    struct {
      smx_process_t process;
      int result;
    } process_is_suspended;

    struct {
      smx_process_t process;
      xbt_dict_t result;
    } process_get_properties;

    struct {
      double duration;
      e_smx_state_t result;
    } process_sleep;

    struct {
      const char *name;
      smx_rdv_t result;
    } rdv_create;

    struct {
      smx_rdv_t rdv;
    } rdv_destroy;

    struct {
      const char* name;
      smx_rdv_t result;
    } rdv_get_by_name;

    struct {
      smx_rdv_t rdv;
      smx_host_t host;
      unsigned int result; 
    } rdv_comm_count_by_host;

    struct {
      smx_rdv_t rdv;
      smx_action_t result;
    } rdv_get_head;

    struct {
      smx_rdv_t rdv;
      double task_size;
      double rate;
      void *src_buff;
      size_t src_buff_size;
      int (*match_fun)(void *, void *);
      void *data;
      double timeout;
    } comm_send;

    struct {
      smx_rdv_t rdv;
      double task_size;
      double rate;
      void *src_buff;
      size_t src_buff_size;
      int (*match_fun)(void *, void *);
      void (*clean_fun)(void *);
      void *data;
      int detached;
      smx_action_t result;
    } comm_isend;

    struct {
      smx_rdv_t rdv;
      void *dst_buff;
      size_t *dst_buff_size;
      int (*match_fun)(void *, void *);
      void *data;
      double timeout;
    } comm_recv;

    struct {
      smx_rdv_t rdv;
      void *dst_buff;
      size_t *dst_buff_size;
      int (*match_fun)(void *, void *);
          void *data;
      smx_action_t result;
    } comm_irecv;

    struct {
      smx_action_t comm;
    } comm_destroy;

    struct {
      smx_action_t comm;
    } comm_cancel;

    struct {
      xbt_dynar_t comms;
      unsigned int result;
    } comm_waitany;

    struct {
      smx_action_t comm;
      double timeout;
    } comm_wait;

    struct {
      smx_action_t comm;
      int result;
    } comm_test;

    struct {
      xbt_dynar_t comms;
      int result;
    } comm_testany;

    struct {
      smx_action_t comm;
      double result;
    } comm_get_remains;

    struct {
      smx_action_t comm;
      e_smx_state_t result;
    } comm_get_state;

    struct {
      smx_action_t comm;
      void *result;
    } comm_get_src_data;

    struct {
      smx_action_t comm;
      void *result;
    } comm_get_dst_data;

    struct {
      smx_action_t comm;
      smx_process_t result;
    } comm_get_src_proc;

    struct {
      smx_action_t comm;
      smx_process_t result;
    } comm_get_dst_proc;

#ifdef HAVE_LATENCY_BOUND_TRACKING
    struct {
      smx_action_t comm;
      int result;
    } comm_is_latency_bounded;
#endif

#ifdef HAVE_TRACING
    struct {
      smx_action_t action;
      const char *category;
    } set_category;
#endif

    struct {
      smx_mutex_t result;
    } mutex_init;

    struct {
      smx_mutex_t mutex;
    } mutex_lock;

    struct {
      smx_mutex_t mutex;
      int result;
    } mutex_trylock;

    struct {
      smx_mutex_t mutex;
    } mutex_unlock;

    struct {
      smx_mutex_t mutex;
    } mutex_destroy;

    struct {
      smx_cond_t result;
    } cond_init;

    struct {
      smx_cond_t cond;
    } cond_destroy;

    struct {
      smx_cond_t cond;
    } cond_signal;

    struct {
      smx_cond_t cond;
      smx_mutex_t mutex;
    } cond_wait;

    struct {
      smx_cond_t cond;
      smx_mutex_t mutex;
      double timeout;
    } cond_wait_timeout;

    struct {
      smx_cond_t cond;
    } cond_broadcast;

    struct {
      int capacity;
      smx_sem_t result;
    } sem_init;

    struct {
      smx_sem_t sem;
    } sem_destroy;

    struct {
      smx_sem_t sem;
    } sem_release;

    struct {
      smx_sem_t sem;
      int result;
    } sem_would_block;

    struct {
      smx_sem_t sem;
    } sem_acquire;

    struct {
      smx_sem_t sem;
      double timeout;
    } sem_acquire_timeout;

    struct {
      smx_sem_t sem;
      int result;
    } sem_get_capacity;
  };
} s_smx_req_t, *smx_req_t;

/******************************** General *************************************/

void SIMIX_request_push(smx_process_t self);
void SIMIX_request_answer(smx_req_t);
void SIMIX_request_pre(smx_req_t, int);
void SIMIX_request_post(smx_action_t);
XBT_INLINE smx_req_t SIMIX_req_mine(void);

#endif

