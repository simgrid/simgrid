/*
 * general.c
 *
 *  Created on: Nov 23, 2009
 *      Author: Lucas Schnorr
 *     License: This program is free software; you can redistribute
 *              it and/or modify it under the terms of the license
 *              (GNU LGPL) which comes with this package.
 *
 *     Copyright (c) 2009 The SimGrid team.
 */

#include "instr/private.h"
#include "instr/config.h"


#ifdef HAVE_TRACING

xbt_dict_t created_containers = NULL;

char *TRACE_paje_msg_container (m_task_t task, char *host, char *output, int len)
{
  if (output){
    snprintf (output, len, "msg-%p-%s-%lld", task, host, task->counter);
    return output;
  }else{
    return NULL;
  }
}

char *TRACE_paje_smx_container (smx_action_t action, int seqnumber, char *host, char *output, int len)
{
  if (output){
    snprintf (output, len, "smx-%p-%d", action, seqnumber);
    return output;
  }else{
    return NULL;
  }
}

char *TRACE_paje_surf_container (void *action, int seqnumber, char *output, int len)
{
  if (output){
    snprintf (output, len, "surf-%p-%d", action, seqnumber);
    return output;
  }else{
    return NULL;
  }
}

char *TRACE_host_container (m_host_t host, char *output, int len)
{
  if (output){
	snprintf (output, len, "%s", MSG_host_get_name(host));
	return output;
  }else{
	return NULL;
  }
}

char *TRACE_task_container (m_task_t task, char *output, int len)
{
  if (output){
	snprintf (output, len, "%p-%lld", task, task->counter);
	return output;
  }else{
	return NULL;
  }
}

char *TRACE_process_container (m_process_t process, char *output, int len)
{
  if (output){
	snprintf (output, len, "%s-%p", MSG_process_get_name(process), process);
	return output;
  }else{
	return NULL;
  }
}

char *TRACE_process_alias_container (m_process_t process, m_host_t host, char *output, int len)
{
  if (output){
     snprintf (output, len, "%p-%p", process, host);
     return output;
  }else{
	 return NULL;
  }
}

char *TRACE_task_alias_container (m_task_t task, m_process_t process, m_host_t host, char *output, int len)
{
  if (output){
	snprintf (output, len, "%p-%lld-%p-%p", task, task->counter, process, host);
	return output;
  }else{
	return NULL;
  }
}

#endif
