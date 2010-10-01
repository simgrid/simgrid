/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_H_
#define INSTR_H_

#include "simgrid_config.h"

#ifdef HAVE_TRACING

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

XBT_PUBLIC(int) TRACE_start (void);
XBT_PUBLIC(int) TRACE_end (void);
XBT_PUBLIC(int) TRACE_category (const char *category);
XBT_PUBLIC(void) TRACE_define_type (const char *type, const char *parent_type, int final);
XBT_PUBLIC(int) TRACE_create_category (const char *category, const char *type, const char *parent_category);
XBT_PUBLIC(void) TRACE_msg_set_task_category (m_task_t task, const char *category);
XBT_PUBLIC(void) TRACE_msg_set_process_category (m_process_t process, const char *category);
XBT_PUBLIC(void) TRACE_set_mask (int mask);
XBT_PUBLIC(void) __TRACE_host_variable (double time, const char *variable, double value, const char *what);
XBT_PUBLIC(void) __TRACE_link_variable (double time, const char *src, const char *dst, const char *variable, double value, const char *what);
XBT_PUBLIC(void) TRACE_declare_mark (const char *mark_type);
XBT_PUBLIC(void) TRACE_mark (const char *mark_type, const char *mark_value);
XBT_PUBLIC(int) TRACE_smpi_set_category (const char *category);

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

#define TRACE_start()
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
#define TRACE_declare_mark(type)
#define TRACE_mark(type,value)
#define TRACE_smpi_set_category(cat)

#endif /* HAVE_TRACING */

#endif /* INSTR_H_ */
