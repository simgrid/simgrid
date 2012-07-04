/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SMURF_PRIVATE_H
#define _SIMIX_SMURF_PRIVATE_H

/********************************* Simcalls *********************************/

/* we want to build the e_smx_simcall_t enumeration and the table of the
 * corresponding strings automatically, using macros */

#define SIMCALL_LIST1 \
SIMCALL_ENUM_ELEMENT(SIMCALL_NONE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_BY_NAME),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_NAME),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_PROPERTIES),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_SPEED),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_AVAILABLE_SPEED),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_STATE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_GET_DATA),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_SET_DATA),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_PARALLEL_EXECUTE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTION_DESTROY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTION_CANCEL),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTION_GET_REMAINS),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTION_GET_STATE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTION_SET_PRIORITY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_HOST_EXECUTION_WAIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_CREATE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_KILL),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_KILLALL),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_CLEANUP),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_CHANGE_HOST),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_SUSPEND),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_RESUME),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_COUNT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_GET_DATA),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_SET_DATA),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_GET_HOST),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_GET_NAME),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_IS_SUSPENDED),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_GET_PROPERTIES),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_SLEEP),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_ON_EXIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_AUTO_RESTART_SET),\
SIMCALL_ENUM_ELEMENT(SIMCALL_PROCESS_RESTART),\
SIMCALL_ENUM_ELEMENT(SIMCALL_RDV_CREATE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_RDV_DESTROY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_RDV_GEY_BY_NAME),\
SIMCALL_ENUM_ELEMENT(SIMCALL_RDV_COMM_COUNT_BY_HOST),\
SIMCALL_ENUM_ELEMENT(SIMCALL_RDV_GET_HEAD),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_SEND),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_ISEND),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_RECV),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_IRECV),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_DESTROY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_CANCEL),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_WAITANY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_WAIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_TEST),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_TESTANY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_GET_REMAINS),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_GET_STATE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_GET_SRC_DATA),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_GET_DST_DATA),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_GET_SRC_PROC),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_GET_DST_PROC),\
SIMCALL_ENUM_ELEMENT(SIMCALL_MUTEX_INIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_MUTEX_DESTROY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_MUTEX_LOCK),\
SIMCALL_ENUM_ELEMENT(SIMCALL_MUTEX_TRYLOCK),\
SIMCALL_ENUM_ELEMENT(SIMCALL_MUTEX_UNLOCK),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COND_INIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COND_DESTROY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COND_SIGNAL),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COND_WAIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COND_WAIT_TIMEOUT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_COND_BROADCAST),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_INIT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_DESTROY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_RELEASE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_WOULD_BLOCK),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_ACQUIRE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_ACQUIRE_TIMEOUT),\
SIMCALL_ENUM_ELEMENT(SIMCALL_SEM_GET_CAPACITY),\
SIMCALL_ENUM_ELEMENT(SIMCALL_FILE_READ),\
SIMCALL_ENUM_ELEMENT(SIMCALL_FILE_WRITE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_FILE_OPEN),\
SIMCALL_ENUM_ELEMENT(SIMCALL_FILE_CLOSE),\
SIMCALL_ENUM_ELEMENT(SIMCALL_FILE_STAT)


/* SIMCALL_COMM_IS_LATENCY_BOUNDED and SIMCALL_SET_CATEGORY make things complicated
 * because they are not always present */
#ifdef HAVE_LATENCY_BOUND_TRACKING
#define SIMCALL_LIST2 \
,SIMCALL_ENUM_ELEMENT(SIMCALL_COMM_IS_LATENCY_BOUNDED)
#else
#define SIMCALL_LIST2
#endif

#ifdef HAVE_TRACING
#define SIMCALL_LIST3 \
,SIMCALL_ENUM_ELEMENT(SIMCALL_SET_CATEGORY)
#else
#define SIMCALL_LIST3
#endif

/* SIMCALL_LIST is the final macro to use */
#define SIMCALL_LIST SIMCALL_LIST1 SIMCALL_LIST2 SIMCALL_LIST3

/* you can redefine the following macro differently to generate something else
 * with the list of enumeration values (e.g. a table of strings or a table of function pointers) */
#define SIMCALL_ENUM_ELEMENT(x) x

/**
 * \brief All possible simcalls.
 */
typedef enum {
SIMCALL_LIST
} e_smx_simcall_t;

/**
 * \brief Represents a simcall to the kernel.
 */
typedef struct s_smx_simcall {
  e_smx_simcall_t call;
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
      double kill_time;
      int argc;
      char **argv;
      xbt_dict_t properties;
      int auto_restart;
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
      smx_process_t process;
      int_f_pvoid_t fun;
      void *data;
    } process_on_exit;

    struct {
      smx_process_t process;
      int auto_restart;
    } process_auto_restart;

    struct {
      smx_process_t process;
    } process_restart;

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
      int (*match_fun)(void *, void *, smx_action_t);
      void *data;
      double timeout;
    } comm_send;

    struct {
      smx_rdv_t rdv;
      double task_size;
      double rate;
      void *src_buff;
      size_t src_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
      void (*clean_fun)(void *);
      void *data;
      int detached;
      smx_action_t result;
    } comm_isend;

    struct {
      smx_rdv_t rdv;
      void *dst_buff;
      size_t *dst_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
      void *data;
      double timeout;
    } comm_recv;

    struct {
      smx_rdv_t rdv;
      void *dst_buff;
      size_t *dst_buff_size;
      int (*match_fun)(void *, void *, smx_action_t);
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

    struct {
      void *ptr;
      size_t size;
      size_t nmemb;
      smx_file_t stream;
      double result;
    } file_read;

    struct {
      const void *ptr;
      size_t size;
      size_t nmemb;
      smx_file_t stream;
      size_t result;
    } file_write;

    struct {
      const char* mount;
      const char* path;
      const char* mode;
      smx_file_t result;
    } file_open;

    struct {
      smx_file_t fp;
      int result;
    } file_close;

    struct {
      smx_file_t fd;
      s_file_stat_t buf;
      int result;
    } file_stat;

  };
} s_smx_simcall_t, *smx_simcall_t;

/******************************** General *************************************/

void SIMIX_simcall_push(smx_process_t self);
void SIMIX_simcall_answer(smx_simcall_t);
void SIMIX_simcall_pre(smx_simcall_t, int);
void SIMIX_simcall_post(smx_action_t);
smx_simcall_t SIMIX_simcall_mine(void);
const char *SIMIX_simcall_name(e_smx_simcall_t kind);

#endif

