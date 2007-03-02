/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_SIMIX_H
#define _SIMIX_SIMIX_H

#include "xbt/misc.h"
#include "simix/datatypes.h"

SG_BEGIN_DECL()


/************************** Global ******************************************/
XBT_PUBLIC(void) SIMIX_config(const char *name, ...);
XBT_PUBLIC(void) SIMIX_global_init(int *argc, char **argv);
XBT_PUBLIC(void) SIMIX_global_init_args(int *argc, char **argv);
XBT_PUBLIC(SIMIX_error_t) SIMIX_main(void);
XBT_PUBLIC(SIMIX_error_t) SIMIX_clean(void);
XBT_PUBLIC(void) SIMIX_function_register(const char *name, smx_process_code_t code);
XBT_PUBLIC(smx_process_code_t) SIMIX_get_registered_function(const char *name);
// SIMIX_deploy_application???
XBT_PUBLIC(void) SIMIX_launch_application(const char *file);

XBT_PUBLIC(double) SIMIX_get_clock(void);

/************************** Host handling ***********************************/
//pointer to above layer
XBT_PUBLIC(SIMIX_error_t) SIMIX_host_set_data(smx_host_t host, void *data);
XBT_PUBLIC(void*) SIMIX_host_get_data(smx_host_t host);

XBT_PUBLIC(const char *) SIMIX_host_get_name(smx_host_t host);
XBT_PUBLIC(smx_host_t) SIMIX_host_self(void);
/* int SIMIX_get_msgload(void); This function lacks specification; discard it */
XBT_PUBLIC(double) SIMIX_get_host_speed(smx_host_t h);
XBT_PUBLIC(int) SIMIX_host_is_avail (smx_host_t h);

XBT_PUBLIC(void) SIMIX_create_environment(const char *file);

XBT_PUBLIC(smx_host_t) SIMIX_get_host_by_name(const char *name);
XBT_PUBLIC(int) SIMIX_get_host_number(void);
XBT_PUBLIC(smx_host_t *)SIMIX_get_host_table(void);

/************************** Process handling *********************************/
XBT_PUBLIC(smx_process_t) SIMIX_process_create(const char *name,
			       smx_process_code_t code, void *data,
			       smx_host_t host);
XBT_PUBLIC(smx_process_t) SIMIX_process_create_with_arguments(const char *name,
					      smx_process_code_t code, void *data,
					      smx_host_t host, int argc, char **argv);
XBT_PUBLIC(void) SIMIX_process_kill(smx_process_t process);
XBT_PUBLIC(void) SIMIX_process_killall(void);

XBT_PUBLIC(SIMIX_error_t) SIMIX_process_change_host(smx_process_t process, smx_host_t host);
//above layer
XBT_PUBLIC(void*) SIMIX_process_get_data(smx_process_t process);
XBT_PUBLIC(SIMIX_error_t) SIMIX_process_set_data(smx_process_t process, void *data);

XBT_PUBLIC(smx_host_t) SIMIX_process_get_host(smx_process_t process);
XBT_PUBLIC(const char *)SIMIX_process_get_name(smx_process_t process);
XBT_PUBLIC(smx_process_t) SIMIX_process_self(void);

XBT_PUBLIC(SIMIX_error_t) SIMIX_process_suspend(smx_process_t process);
XBT_PUBLIC(SIMIX_error_t) SIMIX_process_resume(smx_process_t process);
XBT_PUBLIC(int) SIMIX_process_is_suspended(smx_process_t process);

XBT_PUBLIC(SIMIX_error_t) SIMIX_process_sleep(double nb_sec);
XBT_PUBLIC(SIMIX_error_t) SIMIX_get_errno(void);

/************************** Synchro handling **********************************/

/******Mutex******/
XBT_PUBLIC(smx_mutex_t) SIMIX_mutex_init(void);
XBT_PUBLIC(void) SIMIX_mutex_lock(smx_mutex_t mutex);
XBT_PUBLIC(int) SIMIX_mutex_trylock(smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_mutex_unlock(smx_mutex_t mutex);
XBT_PUBLIC(SIMIX_error_t) SIMIX_mutex_destroy(smx_mutex_t mutex);

/*****Conditional*****/
XBT_PUBLIC(smx_cond_t) SIMIX_cond_init(void);
XBT_PUBLIC(void) SIMIX_cond_signal(smx_cond_t cond);
XBT_PUBLIC(void) SIMIX_cond_wait(smx_cond_t cond,smx_mutex_t mutex);
XBT_PUBLIC(void) SIMIX_cond_wait_timeout(smx_cond_t cond,smx_mutex_t mutex, double max_duration);
XBT_PUBLIC(void) SIMIX_cond_broadcast(smx_cond_t cond);
XBT_PUBLIC(void) SIMIX_cond_destroy(smx_cond_t cond);


/************************** Action handling ************************************/
XBT_PUBLIC(smx_action_t) SIMIX_communicate(smx_host_t sender,smx_host_t receiver, double size);
XBT_PUBLIC(smx_action_t) SIMIX_execute(smx_host_t host,double amount);
XBT_PUBLIC(SIMIX_error_t) SIMIX_action_cancel(smx_action_t action);
XBT_PUBLIC(void) SIMIX_action_set_priority(smx_action_t action, double priority);
XBT_PUBLIC(SIMIX_error_t) SIMIX_action_destroy(smx_action_t action);
XBT_PUBLIC(void) SIMIX_create_link(smx_action_t action, smx_cond_t cond);

//SIMIX_action_wait_for_computation(smx_process_t process, smx_action_t action);

SG_END_DECL()
#endif                          /* _SIMIX_SIMIX_H */
