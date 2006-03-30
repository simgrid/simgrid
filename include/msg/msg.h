/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_H
#define MSG_H

#include "xbt/misc.h"

#include "msg/datatypes.h"
SG_BEGIN_DECL()


/************************** Global ******************************************/
void MSG_config(const char *name, ...);
void MSG_global_init(int *argc, char **argv);
void MSG_global_init_args(int *argc, char **argv);
MSG_error_t MSG_set_channel_number(int number);
int MSG_get_channel_number(void);
MSG_error_t MSG_main(void);
MSG_error_t MSG_clean(void);
void MSG_function_register(const char *name, m_process_code_t code);
m_process_code_t MSG_get_registered_function(const char *name);
void MSG_launch_application(const char *file);
void MSG_paje_output(const char *filename);

double MSG_get_clock(void);

/************************** Host handling ***********************************/
MSG_error_t MSG_host_set_data(m_host_t host, void *data);
void *MSG_host_get_data(m_host_t host);
const char *MSG_host_get_name(m_host_t host);
m_host_t MSG_host_self(void);
int MSG_get_host_msgload(m_host_t host);
/* int MSG_get_msgload(void); This function lacks specification; discard it */
double MSG_get_host_speed(m_host_t h);
int MSG_host_is_avail (m_host_t h);

void MSG_create_environment(const char *file);

m_host_t MSG_get_host_by_name(const char *name);
int MSG_get_host_number(void);
m_host_t *MSG_get_host_table(void);

/************************** Process handling *********************************/
m_process_t MSG_process_create(const char *name,
			       m_process_code_t code, void *data,
			       m_host_t host);
m_process_t MSG_process_create_with_arguments(const char *name,
					      m_process_code_t code, void *data,
					      m_host_t host, int argc, char **argv);
void MSG_process_kill(m_process_t process);
int MSG_process_killall(int reset_PIDs);

MSG_error_t MSG_process_change_host(m_process_t process, m_host_t host);

void *MSG_process_get_data(m_process_t process);
MSG_error_t MSG_process_set_data(m_process_t process, void *data);
m_host_t MSG_process_get_host(m_process_t process);
m_process_t MSG_process_from_PID(int PID);
int MSG_process_get_PID(m_process_t process);
int MSG_process_get_PPID(m_process_t process);
const char *MSG_process_get_name(m_process_t process);
int MSG_process_self_PID(void);
int MSG_process_self_PPID(void);
m_process_t MSG_process_self(void);

MSG_error_t MSG_process_suspend(m_process_t process);
MSG_error_t MSG_process_resume(m_process_t process);
int MSG_process_is_suspended(m_process_t process);

/************************** Task handling ************************************/

m_task_t MSG_task_create(const char *name, double compute_duration,
			 double message_size, void *data);
m_task_t MSG_parallel_task_create(const char *name, 
				  int host_nb,
				  const m_host_t *host_list,
				  double *computation_amount,
				  double *communication_amount,
				  void *data);
void *MSG_task_get_data(m_task_t task);
m_process_t MSG_task_get_sender(m_task_t task);
m_host_t MSG_task_get_source(m_task_t task);
const char *MSG_task_get_name(m_task_t task);
MSG_error_t MSG_task_cancel(m_task_t task);
MSG_error_t MSG_task_destroy(m_task_t task);

MSG_error_t MSG_task_get(m_task_t * task, m_channel_t channel);
MSG_error_t MSG_task_get_with_time_out(m_task_t * task, m_channel_t channel,
				       double max_duration);
MSG_error_t MSG_task_get_from_host(m_task_t * task, int channel, 
				   m_host_t host);
MSG_error_t MSG_task_put(m_task_t task, m_host_t dest, 
			 m_channel_t channel);
MSG_error_t MSG_task_put_bounded(m_task_t task,
				 m_host_t dest, m_channel_t channel,
				 double max_rate);
MSG_error_t MSG_task_execute(m_task_t task);
MSG_error_t MSG_parallel_task_execute(m_task_t task);
void MSG_task_set_priority(m_task_t task, double priority);

int MSG_task_Iprobe(m_channel_t channel);
int MSG_task_probe_from(m_channel_t channel);
int MSG_task_probe_from_host(int channel, m_host_t host);
MSG_error_t MSG_channel_select_from(m_channel_t channel, double max_duration,
				    int *PID);
MSG_error_t MSG_process_sleep(double nb_sec);
MSG_error_t MSG_get_errno(void);

double MSG_task_get_compute_duration(m_task_t task);
double MSG_task_get_remaining_computation(m_task_t task);
double MSG_task_get_data_size(m_task_t task);

SG_END_DECL()
#endif
