/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_H
#define MSG_H

#include "msg/datatypes.h"

/************************** Global ******************************************/
void MSG_global_init(void);
void MSG_global_init_args(int *argc, char **argv);
void MSG_set_verbosity(MSG_outputmode_t mode);
MSG_error_t MSG_set_channel_number(int number);
MSG_error_t MSG_set_sharing_policy(MSG_sharing_t mode, double param);
int MSG_get_channel_number(void);
MSG_error_t MSG_main(void);
MSG_error_t MSG_clean(void);
void MSG_function_register(const char *name, m_process_code_t code);
m_process_code_t MSG_get_registered_function(const char *name);
void MSG_launch_application(const char *file);
void MSG_paje_output(const char *filename);

double MSG_getClock(void);

/************************** Host handling ***********************************/
MSG_error_t MSG_host_set_data(m_host_t host, void *data);
void *MSG_host_get_data(m_host_t host);
const char *MSG_host_get_name(m_host_t host);
m_host_t MSG_host_self(void);
int MSG_get_host_msgload(m_host_t host);
int MSG_get_msgload(void);

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
MSG_error_t MSG_get_arguments(int *argc, char ***argv);
MSG_error_t MSG_set_arguments(m_process_t process,int argc, char *argv[]);

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
int MSG_process_isSuspended(m_process_t process);

MSG_error_t MSG_process_start(m_process_t process);

/************************** Task handling ************************************/

m_task_t MSG_task_create(const char *name, double compute_duration,
			 double message_size, void *data);
void *MSG_task_get_data(m_task_t task);
m_process_t MSG_task_get_sender(m_task_t task);
MSG_error_t MSG_task_destroy(m_task_t task);

MSG_error_t MSG_task_get(m_task_t * task, m_channel_t channel);
MSG_error_t MSG_task_put(m_task_t task, m_host_t dest, 
			 m_channel_t channel);
MSG_error_t MSG_task_put_bounded(m_task_t task,
				 m_host_t dest, m_channel_t channel,
				 double max_rate);
MSG_error_t MSG_task_execute(m_task_t task);
int MSG_task_Iprobe(m_channel_t channel);
int MSG_task_probe_from(m_channel_t channel);
MSG_error_t MSG_process_sleep(double nb_sec);
MSG_error_t MSG_get_errno(void);

/************************** Deprecated ***************************************/
/* MSG_error_t MSG_routing_table_init(void); */
/* MSG_error_t MSG_routing_table_set(m_host_t host1, m_host_t host2, */
/* 				  m_link_t link); */
/* m_link_t MSG_routing_table_get(m_host_t host1, m_host_t host2); */
/* m_host_t MSG_host_create(const char *name, */
/* 			char *trace_file, */
/* 			long double fixed_cpu, */
/* 			char* failure_trace, */
/* 			long double fixed_failure, */
/* 			void *data); */
/* m_host_t MSG_host_from_PID(int PID); */
/* MSG_error_t MSG_host_destroy(m_host_t host); */

/* void MSG_link_set_sharing_value(long double alpha); */
/* m_link_t MSG_link_create(const char *name, */
/* 			 char *lat_trace_file, long double fixed_latency, */
/* 			 char *bw_trace_file, long double fixed_bandwidth); */
/* MSG_error_t MSG_link_destroy(m_link_t link); */
/* m_link_t MSG_link_merge(const char *name, m_link_t src1, m_link_t src2); */
/* m_link_t MSG_get_link_by_name(const char *name); */
/* void MSG_tracelink(m_host_t dest, const char* **names, int *count); */

#endif
