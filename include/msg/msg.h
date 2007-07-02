/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_H
#define MSG_H

#include "xbt.h"

#include "msg/datatypes.h"
SG_BEGIN_DECL()


/************************** Global ******************************************/
XBT_PUBLIC(void) MSG_config(const char *name, ...);
XBT_PUBLIC(void) MSG_global_init(int *argc, char **argv);
XBT_PUBLIC(void) MSG_global_init_args(int *argc, char **argv);
XBT_PUBLIC(MSG_error_t) MSG_set_channel_number(int number);
XBT_PUBLIC(int) MSG_get_channel_number(void);
XBT_PUBLIC(MSG_error_t) MSG_main(void);
XBT_PUBLIC(MSG_error_t) MSG_clean(void);
XBT_PUBLIC(void) MSG_function_register(const char *name, m_process_code_t code);
XBT_PUBLIC(m_process_code_t) MSG_get_registered_function(const char *name);
XBT_PUBLIC(void) MSG_launch_application(const char *file);
XBT_PUBLIC(void) MSG_paje_output(const char *filename);

XBT_PUBLIC(double) MSG_get_clock(void);

/************************** Host handling ***********************************/
XBT_PUBLIC(MSG_error_t) MSG_host_set_data(m_host_t host, void *data);
XBT_PUBLIC(void*) MSG_host_get_data(m_host_t host);
XBT_PUBLIC(const char *) MSG_host_get_name(m_host_t host);
XBT_PUBLIC(m_host_t) MSG_host_self(void);
XBT_PUBLIC(int) MSG_get_host_msgload(m_host_t host);
/* int MSG_get_msgload(void); This function lacks specification; discard it */
XBT_PUBLIC(double) MSG_get_host_speed(m_host_t h);
XBT_PUBLIC(int) MSG_host_is_avail (m_host_t h);

XBT_PUBLIC(void) MSG_create_environment(const char *file);

XBT_PUBLIC(m_host_t) MSG_get_host_by_name(const char *name);
XBT_PUBLIC(int) MSG_get_host_number(void);
XBT_PUBLIC(m_host_t *)MSG_get_host_table(void);

/************************** Process handling *********************************/
XBT_PUBLIC(m_process_t) MSG_process_create(const char *name,
			       m_process_code_t code, void *data,
			       m_host_t host);
XBT_PUBLIC(m_process_t) MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
					      m_host_t host, int argc, char **argv);
XBT_PUBLIC(void) MSG_process_kill(m_process_t process);
XBT_PUBLIC(int) MSG_process_killall(int reset_PIDs);

XBT_PUBLIC(MSG_error_t) MSG_process_change_host(m_process_t process, m_host_t host);

XBT_PUBLIC(void*) MSG_process_get_data(m_process_t process);
XBT_PUBLIC(MSG_error_t) MSG_process_set_data(m_process_t process, void *data);
XBT_PUBLIC(m_host_t) MSG_process_get_host(m_process_t process);
XBT_PUBLIC(m_process_t) MSG_process_from_PID(int PID);
XBT_PUBLIC(int) MSG_process_get_PID(m_process_t process);
XBT_PUBLIC(int) MSG_process_get_PPID(m_process_t process);
XBT_PUBLIC(const char *)MSG_process_get_name(m_process_t process);
XBT_PUBLIC(int) MSG_process_self_PID(void);
XBT_PUBLIC(int) MSG_process_self_PPID(void);
XBT_PUBLIC(m_process_t) MSG_process_self(void);

XBT_PUBLIC(MSG_error_t) MSG_process_suspend(m_process_t process);
XBT_PUBLIC(MSG_error_t) MSG_process_resume(m_process_t process);
XBT_PUBLIC(int) MSG_process_is_suspended(m_process_t process);

/************************** Task handling ************************************/

XBT_PUBLIC(m_task_t) MSG_task_create(const char *name, double compute_duration,
			 double message_size, void *data);
XBT_PUBLIC(m_task_t) MSG_parallel_task_create(const char *name, 
				  int host_nb,
				  const m_host_t *host_list,
				  double *computation_amount,
				  double *communication_amount,
				  void *data);
XBT_PUBLIC(void*) MSG_task_get_data(m_task_t task);
XBT_PUBLIC(m_process_t) MSG_task_get_sender(m_task_t task);
XBT_PUBLIC(m_host_t) MSG_task_get_source(m_task_t task);
XBT_PUBLIC(const char *) MSG_task_get_name(m_task_t task);
XBT_PUBLIC(MSG_error_t) MSG_task_cancel(m_task_t task);
XBT_PUBLIC(MSG_error_t) MSG_task_destroy(m_task_t task);

XBT_PUBLIC(MSG_error_t) MSG_task_get(m_task_t * task, m_channel_t channel);
XBT_PUBLIC(MSG_error_t) MSG_task_get_with_time_out(m_task_t * task, m_channel_t channel,
				       double max_duration);
XBT_PUBLIC(MSG_error_t) MSG_task_get_from_host(m_task_t * task, int channel, 
				   m_host_t host);
XBT_PUBLIC(MSG_error_t) MSG_task_put(m_task_t task, m_host_t dest, 
			 m_channel_t channel);
XBT_PUBLIC(MSG_error_t) MSG_task_put_bounded(m_task_t task,
				 m_host_t dest, m_channel_t channel,
				 double max_rate);
XBT_PUBLIC(MSG_error_t) MSG_task_put_with_timeout(m_task_t task, m_host_t dest, 
				      m_channel_t channel, double max_duration);
XBT_PUBLIC(MSG_error_t) MSG_task_execute(m_task_t task);
XBT_PUBLIC(MSG_error_t) MSG_parallel_task_execute(m_task_t task);
XBT_PUBLIC(void) MSG_task_set_priority(m_task_t task, double priority);

XBT_PUBLIC(int) MSG_task_Iprobe(m_channel_t channel);
XBT_PUBLIC(int) MSG_task_probe_from(m_channel_t channel);
XBT_PUBLIC(int) MSG_task_probe_from_host(int channel, m_host_t host);
XBT_PUBLIC(MSG_error_t) MSG_channel_select_from(m_channel_t channel, double max_duration,
				    int *PID);
XBT_PUBLIC(MSG_error_t) MSG_process_sleep(double nb_sec);
XBT_PUBLIC(MSG_error_t) MSG_get_errno(void);

XBT_PUBLIC(double) MSG_task_get_compute_duration(m_task_t task);
XBT_PUBLIC(double) MSG_task_get_remaining_computation(m_task_t task);
XBT_PUBLIC(double) MSG_task_get_data_size(m_task_t task);

SG_END_DECL()
#endif
