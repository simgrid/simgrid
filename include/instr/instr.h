/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_H_
#define INSTR_H_

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#include "xbt.h"
#include "xbt/graph.h"
#include "msg/msg.h"
#include "simdag/simdag.h"

XBT_PUBLIC(void) TRACE_category(const char *category);
XBT_PUBLIC(void) TRACE_category_with_color (const char *category, const char *color);
XBT_PUBLIC(void) TRACE_msg_set_task_category(m_task_t task,
                                             const char *category);
void TRACE_msg_set_process_category(m_process_t process, const char *category, const char *color);
XBT_PUBLIC(void) TRACE_user_host_variable(double time,
                                          const char *variable,
                                          double value, const char *what);
XBT_PUBLIC(const char *) TRACE_node_name (xbt_node_t node);
XBT_PUBLIC(xbt_graph_t) TRACE_platform_graph (void);
XBT_PUBLIC(void) TRACE_platform_graph_export_graphviz (xbt_graph_t g, const char *filename);
XBT_PUBLIC(void) TRACE_user_link_variable(double time, const char *resource,
                              const char *variable,
                              double value, const char *what);
XBT_PUBLIC(void) TRACE_declare_mark(const char *mark_type);
XBT_PUBLIC(void) TRACE_mark(const char *mark_type, const char *mark_value);
XBT_PUBLIC(void) TRACE_smpi_set_category(const char *category);
XBT_PUBLIC(void) TRACE_sd_set_task_category(SD_task_t task,
                                            const char *category);

#define TRACE_host_variable_declare(var) \
	TRACE_user_host_variable(0,var,0,"declare");

#define TRACE_host_variable_set_with_time(time,var,value) \
	TRACE_user_host_variable(time,var,value,"set");

#define TRACE_host_variable_add_with_time(time,var,value) \
	TRACE_user_host_variable(time,var,value,"add");

#define TRACE_host_variable_sub_with_time(time,var,value) \
	TRACE_user_host_variable(time,var,value,"sub");

#define TRACE_host_variable_set(var,value) \
	TRACE_user_host_variable(MSG_get_clock(),var,value,"set");

#define TRACE_host_variable_add(var,value) \
	TRACE_user_host_variable(MSG_get_clock(),var,value,"add");

#define TRACE_host_variable_sub(var,value) \
	TRACE_user_host_variable(MSG_get_clock(),var,value,"sub");

#define TRACE_link_variable_declare(var) \
	TRACE_user_link_variable(0,NULL,var,0,"declare");

#define TRACE_link_variable_set_with_time(time,link,var,value) \
	TRACE_user_link_variable(time,link,var,value,"set");

#define TRACE_link_variable_add_with_time(time,link,var,value) \
	TRACE_user_link_variable(time,link,var,value,"add");

#define TRACE_link_variable_sub_with_time(time,link,var,value) \
	TRACE_user_link_variable(time,link,var,value,"sub");

#define TRACE_link_variable_set(link,var,value) \
	TRACE_user_link_variable(MSG_get_clock(),link,var,value,"set");

#define TRACE_link_variable_add(link,var,value) \
	TRACE_user_link_variable(MSG_get_clock(),link,var,value,"add");

#define TRACE_link_variable_sub(link,var,value) \
	TRACE_user_link_variable(MSG_get_clock(),link,var,value,"sub");

#else                           /* HAVE_TRACING */

#define TRACE_category(cat)
#define TRACE_category_with_color(cat,color)
#define TRACE_msg_set_task_category(task,cat)
#define TRACE_msg_set_process_category(proc,cat,color)
#define TRACE_set_mask(mask)

#define TRACE_host_variable_declare(var)
#define TRACE_host_variable_set_with_time(time,var,value)
#define TRACE_host_variable_add_with_time(time,var,value)
#define TRACE_host_variable_sub_with_time(time,var,value)
#define TRACE_host_variable_set(var,value)
#define TRACE_host_variable_add(var,value)
#define TRACE_host_variable_sub(var,value)
#define TRACE_link_variable_declare(var)
#define TRACE_link_variable_set_with_time(time,link,var,value)
#define TRACE_link_variable_add_with_time(time,link,var,value)
#define TRACE_link_variable_sub_with_time(time,link,var,value)
#define TRACE_link_variable_set(link,var,value)
#define TRACE_link_variable_add(link,var,value)
#define TRACE_link_variable_sub(link,var,value)
#define TRACE_declare_mark(type)
#define TRACE_mark(type,value)
#define TRACE_smpi_set_category(cat)
#define TRACE_sd_set_task_category(task,cat)

#endif                          /* HAVE_TRACING */

#endif                          /* INSTR_H_ */
