/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/private.h"

#ifdef HAVE_TRACING

void __TRACE_msg_volume_start (m_task_t task)
{
	m_process_t process = NULL;
	m_host_t host = NULL;
	char process_name[200], process_alias[200];
	char task_name[200];
	double volume = 0;
  if (!IS_TRACING_VOLUME) return;

  /* check if task is traced */
  if (!IS_TRACED(task)) return;

  /* check if process is traced */
  process = MSG_process_self ();
  if (!IS_TRACED(process)) return;

  host = MSG_process_get_host (process);
  TRACE_process_container (process, process_name, 200);
  TRACE_process_alias_container (process, host, process_alias, 200);
  TRACE_task_container (task, task_name, 200);

  volume = MSG_task_get_data_size (task);

  pajeStartLinkWithVolume (MSG_get_clock(), "volume", "0", task->category, process_alias, task_name, volume);
}

void __TRACE_msg_volume_finish (m_task_t task)
{
	char process_name[200], process_alias[200];
	char task_name[200];
	m_process_t process = NULL;
	m_host_t host = NULL;
  if (!IS_TRACING_VOLUME) return;

  /* check if task is traced */
  if (!IS_TRACED(task)) return;

  /* check if process is traced */
  process = MSG_process_self ();
  if (!IS_TRACED(process)) return;

  host = MSG_process_get_host (process);
  TRACE_process_container (process, process_name, 200);
  TRACE_process_alias_container (process, host, process_alias, 200);
  TRACE_task_container (task, task_name, 200);

  pajeEndLink (MSG_get_clock(), "volume", "0", task->category, process_alias, task_name);
}

#endif
