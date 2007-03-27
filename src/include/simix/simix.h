/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SIMIX_H
#define _SIMIX_SIMIX_H

#include "xbt/misc.h"
#include "xbt/fifo.h"
#include "simix/datatypes.h"

SG_BEGIN_DECL()


/************************** Global ******************************************/
XBT_PUBLIC(void) SIMIX_config(const char *name, va_list pa);
XBT_PUBLIC(void) SIMIX_global_init(int *argc, char **argv);
XBT_PUBLIC(void) SIMIX_global_init_args(int *argc, char **argv);
XBT_PUBLIC(void) SIMIX_clean(void);
XBT_PUBLIC(void) SIMIX_function_register(const char *name, smx_process_code_t code);
XBT_PUBLIC(smx_process_code_t) SIMIX_get_registered_function(const char *name);

XBT_PUBLIC(void) SIMIX_launch_application(const char *file);

XBT_PUBLIC(double) SIMIX_get_clock(void);
XBT_PUBLIC(double) SIMIX_solve(xbt_fifo_t actions_done, xbt_fifo_t actions_failed);

/* Timer functions */
XBT_PUBLIC(void) SIMIX_timer_set (double date, void *function, void *arg);
XBT_PUBLIC(int) SIMIX_timer_get(void **function, void **arg);

/* only for tests */
XBT_PUBLIC(void) __SIMIX_main(void);

/* User create and kill process, the function must accept the folling parameters:
 * const char *name: a name for the object. It is for user-level information and can be NULL
 * smx_process_code_t code: is a function describing the behavior of the agent
 * void *data: data a pointer to any data one may want to attach to the new object.
 * smx_host_t host: the location where the new agent is executed
 * int argc, char **argv: parameters passed to code
 *
 * */
XBT_PUBLIC(void) SIMIX_function_register_process_create(void * function);
XBT_PUBLIC(void) SIMIX_function_register_process_kill(void * function);

/************************** Host handling ***********************************/

XBT_PUBLIC(void) SIMIX_host_set_data(smx_host_t host, void *data);
XBT_PUBLIC(void*) SIMIX_host_get_data(smx_host_t host);

XBT_PUBLIC(const char *) SIMIX_host_get_name(smx_host_t host);
XBT_PUBLIC(smx_host_t) SIMIX_host_self(void);
XBT_PUBLIC(double) SIMIX_host_get_speed(smx_host_t host);
XBT_PUBLIC(double) SIMIX_host_get_available_speed(smx_host_t host);

XBT_PUBLIC(int) SIMIX_host_get_number(void);
XBT_PUBLIC(smx_host_t *)SIMIX_host_get_table(void);

XBT_PUBLIC(void) SIMIX_create_environment(const char *file);
XBT_PUBLIC(smx_host_t) SIMIX_host_get_by_name(const char *name);

/* Two possible states, 1 - CPU ON and 0 CPU OFF */
XBT_PUBLIC(int) SIMIX_host_get_state(smx_host_t host);

/************************** Process handling *********************************/
XBT_PUBLIC(smx_process_t) SIMIX_process_create(const char *name,
			       smx_process_code_t code, void *data,
			       const char * hostname, void * clean_process_function);
XBT_PUBLIC(smx_process_t) SIMIX_process_create_with_arguments(const char *name,
					      smx_process_code_t code, void *data,
					      const char * hostname, int argc, char **argv, void * clean_process_function);
XBT_PUBLIC(void) SIMIX_process_kill(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_cleanup(void *arg);
XBT_PUBLIC(void) SIMIX_process_killall(void);

//above layer
XBT_PUBLIC(void*) SIMIX_process_get_data(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_set_data(smx_process_t process, void *data);

XBT_PUBLIC(smx_host_t) SIMIX_process_get_host(smx_process_t process);
XBT_PUBLIC(const char *)SIMIX_process_get_name(smx_process_t process);
XBT_PUBLIC(smx_process_t) SIMIX_process_self(void);

XBT_PUBLIC(void) SIMIX_process_suspend(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_resume(smx_process_t process);
XBT_PUBLIC(int) SIMIX_process_is_suspended(smx_process_t process);


/************************** Synchro handling **********************************/

/******Mutex******/
XBT_PUBLIC(smx_mutex_t) SIMIX_mutex_init(void);
XBT_PUBLIC(void) SIMIX_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC(int) SIMIX_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_mutex_unlock(smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_mutex_destroy(smx_mutex_t mutex);

/*****Conditional*****/
XBT_PUBLIC(smx_cond_t) SIMIX_cond_init(void);
XBT_PUBLIC(void) SIMIX_cond_signal(smx_cond_t cond);
XBT_PUBLIC(void) SIMIX_cond_wait(smx_cond_t cond,smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_cond_wait_timeout(smx_cond_t cond,smx_mutex_t mutex, double max_duration);
XBT_PUBLIC(void) SIMIX_cond_broadcast(smx_cond_t cond);
XBT_PUBLIC(void) SIMIX_cond_destroy(smx_cond_t cond);
XBT_PUBLIC(void) SIMIX_register_condition_to_action(smx_action_t action, smx_cond_t cond);


/************************** Action handling ************************************/
XBT_PUBLIC(smx_action_t) SIMIX_action_communicate(smx_host_t sender,smx_host_t receiver, char *name,
																									double size, double rate);
XBT_PUBLIC(smx_action_t) SIMIX_action_execute(smx_host_t host,char *name, double amount);
XBT_PUBLIC(smx_action_t) SIMIX_action_sleep(smx_host_t host, double amount);
XBT_PUBLIC(void) SIMIX_action_cancel(smx_action_t action);
XBT_PUBLIC(void) SIMIX_action_set_priority(smx_action_t action, double priority);
XBT_PUBLIC(void) SIMIX_action_destroy(smx_action_t action);
XBT_PUBLIC(void) SIMIX_register_action_to_condition(smx_action_t action, smx_cond_t cond);
XBT_PUBLIC(double) SIMIX_action_get_remains(smx_action_t action);

/*Not implemented yet */
XBT_PUBLIC(smx_action_t) SIMIX_action_parallel_execute(int workstation_nb,      
												 															void **workstation_list,
																											double *computation_amount,
																											double *communication_amount,
																											double amount,
																											double rate);


SG_END_DECL()

#endif                          /* _SIMIX_SIMIX_H */
