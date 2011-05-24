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
XBT_PUBLIC(void) TRACE_msg_set_task_category(m_task_t task, const char *category);
XBT_PUBLIC(void) TRACE_msg_set_process_category(m_process_t process, const char *category, const char *color);
XBT_PUBLIC(void) TRACE_smpi_set_category(const char *category);
XBT_PUBLIC(void) TRACE_sd_set_task_category(SD_task_t task, const char *category);

XBT_PUBLIC(void) TRACE_declare_mark(const char *mark_type);
XBT_PUBLIC(void) TRACE_mark(const char *mark_type, const char *mark_value);

XBT_PUBLIC(const char *) TRACE_node_name (xbt_node_t node);
XBT_PUBLIC(xbt_graph_t) TRACE_platform_graph (void);
XBT_PUBLIC(void) TRACE_platform_graph_export_graphviz (xbt_graph_t g, const char *filename);


/*
 * User-variables related functions
 */
typedef enum {
  INSTR_US_DECLARE,
  INSTR_US_SET,
  INSTR_US_ADD,
  INSTR_US_SUB,
} InstrUserVariable;

XBT_PUBLIC(void) TRACE_user_variable(double time,
                              const char *resource,
                              const char *variable,
                              const char *father_type,
                              double value,
                              InstrUserVariable what);
XBT_PUBLIC(void) TRACE_user_srcdst_variable(double time,
                              const char *src,
                              const char *dst,
                              const char *variable,
                              const char *father_type,
                              double value,
                              InstrUserVariable what);

#define TRACE_host_variable_declare(var) \
	TRACE_user_variable(0,NULL,var,"HOST",0,INSTR_US_DECLARE);

#define TRACE_host_variable_set_with_time(time,host,var,value) \
	TRACE_user_variable(time,host,var,"HOST",value,INSTR_US_SET);

#define TRACE_host_variable_add_with_time(time,host,var,value) \
	TRACE_user_variable(time,host,var,"HOST",value,INSTR_US_ADD);

#define TRACE_host_variable_sub_with_time(time,host,var,value) \
	TRACE_user_variable(time,host,var,"HOST",value,INSTR_US_SUB);

#define TRACE_host_variable_set(host,var,value) \
	TRACE_user_variable(MSG_get_clock(),host,var,"HOST",value,INSTR_US_SET);

#define TRACE_host_variable_add(host,var,value) \
	TRACE_user_variable(MSG_get_clock(),host,var,"HOST",value,INSTR_US_ADD);

#define TRACE_host_variable_sub(host,var,value) \
	TRACE_user_variable(MSG_get_clock(),host,var,"HOST",value,INSTR_US_SUB);

#define TRACE_link_variable_declare(var) \
	TRACE_user_variable(0,NULL,var,"LINK",0,INSTR_US_DECLARE);

#define TRACE_link_variable_set_with_time(time,link,var,value) \
	TRACE_user_variable(time,link,var,"LINK",value,INSTR_US_SET);

#define TRACE_link_variable_add_with_time(time,link,var,value) \
	TRACE_user_variable(time,link,var,"LINK",value,INSTR_US_ADD);

#define TRACE_link_variable_sub_with_time(time,link,var,value) \
	TRACE_user_variable(time,link,var,"LINK",value,INSTR_US_SUB);

#define TRACE_link_variable_set(link,var,value) \
	TRACE_user_variable(MSG_get_clock(),link,var,"LINK",value,INSTR_US_SET);

#define TRACE_link_variable_add(link,var,value) \
	TRACE_user_variable(MSG_get_clock(),link,var,"LINK",value,INSTR_US_ADD);

#define TRACE_link_variable_sub(link,var,value) \
	TRACE_user_variable(MSG_get_clock(),link,var,"LINK",value,INSTR_US_SUB);

//user provides src and dst, we set the value for the variables of all
//links connecting src and dst
#define TRACE_link_srcdst_variable_set_with_time(time,src,dst,var,value) \
  TRACE_user_srcdst_variable(time,src,dst,,var,"LINK",value,INSTR_US_SET);

#define TRACE_link_srcdst_variable_add_with_time(time,src,dst,var,value) \
  TRACE_user_srcdst_variable(time,src,dst,var,"LINK",value,INSTR_US_ADD);

#define TRACE_link_srcdst_variable_sub_with_time(time,src,dst,var,value) \
  TRACE_user_srcdst_variable(time,src,dst,var,"LINK",value,INSTR_US_SUB);

#define TRACE_link_srcdst_variable_set(src,dst,var,value) \
  TRACE_user_srcdst_variable(MSG_get_clock(),src,dst,var,"LINK",value,INSTR_US_SET);

#define TRACE_link_srcdst_variable_add(src,dst,var,value) \
  TRACE_user_srcdst_variable(MSG_get_clock(),src,dst,var,"LINK",value,INSTR_US_ADD);

#define TRACE_link_srcdst_variable_sub(src,dst,var,value) \
  TRACE_user_srcdst_variable(MSG_get_clock(),src,dst,var,"LINK",value,INSTR_US_SUB);

#else                           /* HAVE_TRACING */

#define TRACE_category(category)
#define TRACE_category_with_color(category,color)
#define TRACE_msg_set_task_category(task,category)
#define TRACE_msg_set_process_category(process,category,color)
#define TRACE_smpi_set_category(category)
#define TRACE_sd_set_task_category(task,category)
#define TRACE_declare_mark(mark_type)
#define TRACE_mark(mark_type,mark_value)
#define TRACE_node_name(node)
#define TRACE_platform_graph(void)
#define TRACE_platform_graph_export_graphviz(g,filename)

#define TRACE_host_variable_declare(var)
#define TRACE_host_variable_set_with_time(time,host,var,value)
#define TRACE_host_variable_add_with_time(time,host,var,value)
#define TRACE_host_variable_sub_with_time(time,host,var,value)
#define TRACE_host_variable_set(host,var,value)
#define TRACE_host_variable_add(host,var,value)
#define TRACE_host_variable_sub(host,var,value)
#define TRACE_link_variable_declare(var)
#define TRACE_link_variable_set_with_time(time,link,var,value)
#define TRACE_link_variable_add_with_time(time,link,var,value)
#define TRACE_link_variable_sub_with_time(time,link,var,value)
#define TRACE_link_variable_set(link,var,value)
#define TRACE_link_variable_add(link,var,value)
#define TRACE_link_variable_sub(link,var,value)
#define TRACE_link_srcdst_variable_set_with_time(time,src,dst,var,value)
#define TRACE_link_srcdst_variable_add_with_time(time,src,dst,var,value)
#define TRACE_link_srcdst_variable_sub_with_time(time,src,dst,var,value)
#define TRACE_link_srcdst_variable_set(src,dst,var,value)
#define TRACE_link_srcdst_variable_add(src,dst,var,value)
#define TRACE_link_srcdst_variable_sub(src,dst,var,value)



#endif                          /* HAVE_TRACING */

#endif                          /* INSTR_H_ */
