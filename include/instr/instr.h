/*
 * instr.h
 *
 *  Created on: Nov 23, 2009
 *      Author: Lucas Schnorr
 *     License: This program is free software; you can redistribute
 *              it and/or modify it under the terms of the license
 *              (GNU LGPL) which comes with this package.
 *
 *     Copyright (c) 2009 The SimGrid team.
 */

#ifndef INSTR_H_
#define INSTR_H_

#include "instr/tracing_config.h"

#ifdef HAVE_TRACING

#define NO_TRACE		0
#define TRACE_PLATFORM  1
#define TRACE_PROCESS   2
#define TRACE_TASK      4

#include "xbt.h"
#include "msg/msg.h"

/* Trace error codes (used in exceptions) */
#define TRACE_ERROR_COMPLEX_ROUTES 100
#define TRACE_ERROR_TYPE_NOT_DEFINED 200
#define TRACE_ERROR_TYPE_ALREADY_DEFINED 201
#define TRACE_ERROR_CATEGORY_NOT_DEFINED 300
#define TRACE_ERROR_CATEGORY_ALREADY_DEFINED 301
#define TRACE_ERROR_MASK 400
#define TRACE_ERROR_FILE_OPEN 401
#define TRACE_ERROR_START 500

XBT_PUBLIC(int) TRACE_start_with_mask (const char *filename, int mask);
XBT_PUBLIC(int) TRACE_end (void);
XBT_PUBLIC(int) TRACE_category (const char *category);
XBT_PUBLIC(void) TRACE_define_type (const char *type, const char *parent_type, int final);
XBT_PUBLIC(int) TRACE_create_category (const char *category, const char *type, const char *parent_category);
XBT_PUBLIC(void) TRACE_msg_set_task_category (m_task_t task, const char *category);
XBT_PUBLIC(void) TRACE_msg_set_process_category (m_process_t process, const char *category);
XBT_PUBLIC(void) TRACE_set_mask (int mask);
XBT_PUBLIC(void) __TRACE_host_variable (double time, const char *variable, double value, const char *what);
XBT_PUBLIC(void) __TRACE_link_variable (double time, const char *src, const char *dst, const char *variable, double value, const char *what);

#define TRACE_start(filename) TRACE_start_with_mask(filename,TRACE_PLATFORM)

#define TRACE_host_variable_declare(var) \
	__TRACE_host_variable(0,var,0,"declare");

#define TRACE_host_variable_set_with_time(time,var,value) \
	__TRACE_host_variable(time,var,value,"set");

#define TRACE_host_variable_add_with_time(time,var,value) \
	__TRACE_host_variable(time,var,value,"add");

#define TRACE_host_variable_sub_with_time(time,var,value) \
	__TRACE_host_variable(time,var,value,"sub");

#define TRACE_host_variable_set(var,value) \
	__TRACE_host_variable(MSG_get_clock(),var,value,"set");

#define TRACE_host_variable_add(var,value) \
	__TRACE_host_variable(MSG_get_clock(),var,value,"add");

#define TRACE_host_variable_sub(var,value) \
	__TRACE_host_variable(MSG_get_clock(),var,value,"sub");

#define TRACE_link_variable_declare(var) \
	__TRACE_link_variable(0,NULL,NULL,var,0,"declare");

#define TRACE_link_variable_set_with_time(time,src,dst,var,value) \
	__TRACE_link_variable(time,src,dst,var,value,"set");

#define TRACE_link_variable_add_with_time(time,src,dst,var,value) \
	__TRACE_link_variable(time,src,dst,var,value,"add");

#define TRACE_link_variable_sub_with_time(time,src,dst,var,value) \
	__TRACE_link_variable(time,src,dst,var,value,"sub");

#define TRACE_link_variable_set(src,dst,var,value) \
	__TRACE_link_variable(MSG_get_clock(),src,dst,var,value,"set");

#define TRACE_link_variable_add(src,dst,var,value) \
	__TRACE_link_variable(MSG_get_clock(),src,dst,var,value,"add");

#define TRACE_link_variable_sub(src,dst,var,value) \
	__TRACE_link_variable(MSG_get_clock(),src,dst,var,value,"sub");

#else /* HAVE_TRACING */

#define TRACE_start(filename)
#define TRACE_start_with_mask(filename,mask)
#define TRACE_end()
#define TRACE_category(cat)
#define TRACE_define_type(cat,supercat,final)
#define TRACE_create_category(inst,cat)
#define TRACE_msg_set_task_category(task,cat)
#define TRACE_msg_set_process_category(proc,cat)
#define TRACE_set_mask(mask)

#define TRACE_host_variable_declare(var)
#define TRACE_host_variable_set_with_time(time,var,value)
#define TRACE_host_variable_add_with_time(time,var,value)
#define TRACE_host_variable_sub_with_time(time,var,value)
#define TRACE_host_variable_set(var,value)
#define TRACE_host_variable_add(var,value)
#define TRACE_host_variable_sub(var,value)
#define TRACE_link_variable_declare(var)
#define TRACE_link_variable_set_with_time(time,src,dst,var,value)
#define TRACE_link_variable_add_with_time(time,src,dst,var,value)
#define TRACE_link_variable_sub_with_time(time,src,dst,var,value)
#define TRACE_link_variable_set(src,dst,var,value)
#define TRACE_link_variable_add(src,dst,var,value)
#define TRACE_link_variable_sub(src,dst,var,value)

#endif /* HAVE_TRACING */

#endif /* INSTR_H_ */
