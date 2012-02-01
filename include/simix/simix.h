/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SIMIX_H
#define _SIMIX_SIMIX_H

#include "xbt/misc.h"
#include "xbt/fifo.h"
#include "xbt/dict.h"
#include "xbt/function_types.h"
#include "simix/datatypes.h"
#include "simix/context.h"

SG_BEGIN_DECL()

/********************************** Global ************************************/
/* Initialization and exit */
XBT_PUBLIC(void) SIMIX_global_init(int *argc, char **argv);
XBT_PUBLIC(void) SIMIX_clean(void);


XBT_PUBLIC(void) SIMIX_function_register_process_cleanup(void_pfn_smxprocess_t function);
XBT_PUBLIC(void) SIMIX_function_register_process_create(smx_creation_func_t function);
XBT_PUBLIC(void) SIMIX_function_register_process_kill(void_pfn_smxprocess_t function);

/* Simulation execution */
XBT_PUBLIC(void) SIMIX_run(void);    
XBT_INLINE XBT_PUBLIC(double) SIMIX_get_clock(void);

/* Timer functions FIXME: should these be public? */
XBT_PUBLIC(void) SIMIX_timer_set(double date, void *function, void *arg);
XBT_PUBLIC(double) SIMIX_timer_next(void);

XBT_PUBLIC(void) SIMIX_display_process_status(void);

/******************************* Environment **********************************/
XBT_PUBLIC(void) SIMIX_create_environment(const char *file);

/******************************** Deployment **********************************/

XBT_PUBLIC(void) SIMIX_function_register(const char *name, xbt_main_func_t code);
XBT_PUBLIC(void) SIMIX_function_register_default(xbt_main_func_t code);
XBT_PUBLIC(xbt_main_func_t) SIMIX_get_registered_function(const char *name);
XBT_PUBLIC(void) SIMIX_launch_application(const char *file);
XBT_PUBLIC(void) SIMIX_process_set_function(const char* process_host,
                                            const char *process_function,
                                            xbt_dynar_t arguments,
                                            double process_start_time,
                                            double process_kill_time);

/*********************************** Host *************************************/
XBT_PUBLIC(xbt_dict_t) SIMIX_host_get_dict(void);
XBT_PUBLIC(smx_host_t) SIMIX_host_get_by_name(const char *name);
XBT_PUBLIC(smx_host_t) SIMIX_host_self(void);
XBT_PUBLIC(const char*) SIMIX_host_self_get_name(void);
XBT_PUBLIC(const char*) SIMIX_host_get_name(smx_host_t host); /* FIXME: make private: only the name of SIMIX_host_self() should be public without request */
XBT_PUBLIC(void) SIMIX_host_self_set_data(void *data);
XBT_PUBLIC(void*) SIMIX_host_self_get_data(void);

/********************************* Process ************************************/
XBT_PUBLIC(int) SIMIX_process_count(void);
XBT_INLINE XBT_PUBLIC(smx_process_t) SIMIX_process_self(void);
XBT_PUBLIC(const char*) SIMIX_process_self_get_name(void);
XBT_PUBLIC(void) SIMIX_process_self_set_data(smx_process_t self, void *data);
XBT_PUBLIC(void*) SIMIX_process_self_get_data(smx_process_t self);
XBT_PUBLIC(smx_context_t) SIMIX_process_get_context(smx_process_t);
XBT_PUBLIC(void) SIMIX_process_set_context(smx_process_t p,smx_context_t c);
XBT_PUBLIC(int) SIMIX_process_has_pending_comms(smx_process_t process);

/****************************** Communication *********************************/
XBT_PUBLIC(void) SIMIX_comm_set_copy_data_callback(void (*callback) (smx_action_t, void*, size_t));
XBT_PUBLIC(void) SIMIX_comm_copy_pointer_callback(smx_action_t comm, void* buff, size_t buff_size);
XBT_PUBLIC(void) SIMIX_comm_copy_buffer_callback(smx_action_t comm, void* buff, size_t buff_size);
XBT_PUBLIC(void) smpi_comm_copy_data_callback(smx_action_t comm, void* buff, size_t buff_size);

XBT_PUBLIC(smx_action_t) SIMIX_comm_get_send_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_send_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(int) SIMIX_comm_has_recv_match(smx_rdv_t rdv, int (*match_fun)(void*, void*), void* data);
XBT_PUBLIC(void) SIMIX_comm_finish(smx_action_t action);

/******************************************************************************/
/*                            SIMIX simcalls                                  */
/******************************************************************************/
/* These functions are a system call-like interface to the simulation kernel. */
/* They can also be called from maestro's context, and they are thread safe.  */
/******************************************************************************/

/******************************* Host simcalls ********************************/
/* TODO use handlers and keep smx_host_t hidden from higher levels */
XBT_PUBLIC(xbt_dict_t) simcall_host_get_dict(void);
XBT_INLINE XBT_PUBLIC(smx_host_t) simcall_host_get_by_name(const char *name);
XBT_PUBLIC(const char *) simcall_host_get_name(smx_host_t host);
XBT_PUBLIC(xbt_dict_t) simcall_host_get_properties(smx_host_t host);
XBT_PUBLIC(double) simcall_host_get_speed(smx_host_t host);
XBT_PUBLIC(double) simcall_host_get_available_speed(smx_host_t host);
/* Two possible states, 1 - CPU ON and 0 CPU OFF */
XBT_PUBLIC(int) simcall_host_get_state(smx_host_t host);
XBT_PUBLIC(void *) simcall_host_get_data(smx_host_t host);

XBT_PUBLIC(void) simcall_host_set_data(smx_host_t host, void *data);

XBT_PUBLIC(smx_action_t) simcall_host_execute(const char *name, smx_host_t host,
                                                double computation_amount,
                                                double priority);
XBT_PUBLIC(smx_action_t) simcall_host_parallel_execute(const char *name,
                                                     int host_nb,
                                                     smx_host_t *host_list,
                                                     double *computation_amount,
                                                     double *communication_amount,
                                                     double amount,
                                                     double rate);
XBT_PUBLIC(void) simcall_host_execution_destroy(smx_action_t execution);
XBT_PUBLIC(void) simcall_host_execution_cancel(smx_action_t execution);
XBT_PUBLIC(double) simcall_host_execution_get_remains(smx_action_t execution);
XBT_PUBLIC(e_smx_state_t) simcall_host_execution_get_state(smx_action_t execution);
XBT_PUBLIC(void) simcall_host_execution_set_priority(smx_action_t execution, double priority);
XBT_PUBLIC(e_smx_state_t) simcall_host_execution_wait(smx_action_t execution);


/**************************** Process simcalls ********************************/
/* Constructor and Destructor */
XBT_PUBLIC(void) simcall_process_create(smx_process_t *process,
                                          const char *name,
                                          xbt_main_func_t code,
                                          void *data,
                                          const char *hostname,
                                          int argc, char **argv,
                                          xbt_dict_t properties);

XBT_PUBLIC(void) simcall_process_kill(smx_process_t process);
XBT_PUBLIC(void) simcall_process_killall(void);

/* Process handling */
XBT_PUBLIC(void) simcall_process_cleanup(smx_process_t process);
XBT_PUBLIC(void) simcall_process_change_host(smx_process_t process,
					       smx_host_t dest);
XBT_PUBLIC(void) simcall_process_suspend(smx_process_t process);
XBT_PUBLIC(void) simcall_process_resume(smx_process_t process);

/* Getters and Setters */
XBT_PUBLIC(int) simcall_process_count(void);
XBT_PUBLIC(void *) simcall_process_get_data(smx_process_t process);
XBT_PUBLIC(void) simcall_process_set_data(smx_process_t process, void *data);
XBT_INLINE XBT_PUBLIC(smx_host_t) simcall_process_get_host(smx_process_t process);
XBT_PUBLIC(const char *) simcall_process_get_name(smx_process_t process);
XBT_PUBLIC(int) simcall_process_is_suspended(smx_process_t process);
XBT_PUBLIC(xbt_dict_t) simcall_process_get_properties(smx_process_t host);

/* Sleep control */
XBT_PUBLIC(e_smx_state_t) simcall_process_sleep(double duration);

/************************** Comunication simcalls *****************************/
/***** Rendez-vous points *****/

XBT_PUBLIC(smx_rdv_t) simcall_rdv_create(const char *name);
XBT_PUBLIC(void) simcall_rdv_destroy(smx_rdv_t rvp);
XBT_PUBLIC(smx_rdv_t) simcall_rdv_get_by_name(const char *name);
XBT_PUBLIC(int) simcall_rdv_comm_count_by_host(smx_rdv_t rdv, smx_host_t host);
XBT_PUBLIC(smx_action_t) simcall_rdv_get_head(smx_rdv_t rdv);

XBT_PUBLIC(xbt_dict_t) SIMIX_get_rdv_points(void);

/***** Communication simcalls *****/

XBT_PUBLIC(void) simcall_comm_send(smx_rdv_t rdv, double task_size,
                                     double rate, void *src_buff,
                                     size_t src_buff_size,
                                     int (*match_fun)(void *, void *),
                                     void *data, double timeout);

XBT_PUBLIC(smx_action_t) simcall_comm_isend(smx_rdv_t rdv, double task_size,
                                              double rate, void *src_buff,
                                              size_t src_buff_size,
                                              int (*match_fun)(void *, void *),
                                              void (*clean_fun)(void *),
                                              void *data, int detached);

XBT_PUBLIC(void) simcall_comm_recv(smx_rdv_t rdv, void *dst_buff,
                                     size_t * dst_buff_size,
                                     int (*match_fun)(void *, void *),
                                     void *data, double timeout);

XBT_PUBLIC(smx_action_t) simcall_comm_irecv(smx_rdv_t rdv, void *dst_buff,
                                              size_t * dst_buff_size,
                                              int (*match_fun)(void *, void *),
                                              void *data);

XBT_PUBLIC(void) simcall_comm_destroy(smx_action_t comm);

XBT_INLINE XBT_PUBLIC(void) simcall_comm_cancel(smx_action_t comm);

/* FIXME: waitany is going to be a vararg function, and should take a timeout */
XBT_PUBLIC(unsigned int) simcall_comm_waitany(xbt_dynar_t comms);
XBT_PUBLIC(void) simcall_comm_wait(smx_action_t comm, double timeout);
XBT_PUBLIC(int) simcall_comm_test(smx_action_t comm);
XBT_PUBLIC(int) simcall_comm_testany(xbt_dynar_t comms);

/* Getters and setters */
XBT_PUBLIC(double) simcall_comm_get_remains(smx_action_t comm);
XBT_PUBLIC(e_smx_state_t) simcall_comm_get_state(smx_action_t comm);
XBT_PUBLIC(void *) simcall_comm_get_src_data(smx_action_t comm);
XBT_PUBLIC(void *) simcall_comm_get_dst_data(smx_action_t comm);
XBT_PUBLIC(smx_process_t) simcall_comm_get_src_proc(smx_action_t comm);
XBT_PUBLIC(smx_process_t) simcall_comm_get_dst_proc(smx_action_t comm);

#ifdef HAVE_LATENCY_BOUND_TRACKING
XBT_PUBLIC(int) simcall_comm_is_latency_bounded(smx_action_t comm);
#endif

#ifdef HAVE_TRACING
/************************** Tracing handling **********************************/
XBT_PUBLIC(void) simcall_set_category(smx_action_t action, const char *category);
#endif

/************************** Synchro simcalls **********************************/

XBT_PUBLIC(smx_mutex_t) simcall_mutex_init(void);
XBT_PUBLIC(void) simcall_mutex_destroy(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC(int) simcall_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_mutex_unlock(smx_mutex_t mutex);

XBT_PUBLIC(smx_cond_t) simcall_cond_init(void);
XBT_PUBLIC(void) simcall_cond_destroy(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_signal(smx_cond_t cond);
XBT_PUBLIC(void) simcall_cond_wait(smx_cond_t cond, smx_mutex_t mutex);
XBT_PUBLIC(void) simcall_cond_wait_timeout(smx_cond_t cond,
                                         smx_mutex_t mutex,
                                         double max_duration);
XBT_PUBLIC(void) simcall_cond_broadcast(smx_cond_t cond);

XBT_PUBLIC(smx_sem_t) simcall_sem_init(int capacity);
XBT_PUBLIC(void) simcall_sem_destroy(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_release(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_release_forever(smx_sem_t sem);
XBT_PUBLIC(int) simcall_sem_would_block(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_block_onto(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire(smx_sem_t sem);
XBT_PUBLIC(void) simcall_sem_acquire_timeout(smx_sem_t sem,
                                           double max_duration);
XBT_PUBLIC(unsigned int) simcall_sem_acquire_any(xbt_dynar_t sems);
XBT_PUBLIC(int) simcall_sem_get_capacity(smx_sem_t sem);

XBT_PUBLIC(size_t) simcall_file_read(void* ptr, size_t size, size_t nmemb, smx_file_t stream);
XBT_PUBLIC(size_t) simcall_file_write(const void* ptr, size_t size, size_t nmemb, smx_file_t stream);
XBT_PUBLIC(smx_file_t) simcall_file_open(const char* path, const char* mode);
XBT_PUBLIC(int) simcall_file_close(smx_file_t fp);
XBT_PUBLIC(int) simcall_file_stat(int fd, void* buf);

SG_END_DECL()
#endif                          /* _SIMIX_SIMIX_H */
