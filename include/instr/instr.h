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

/*
 * Functions to manage tracing categories
 */
XBT_PUBLIC(void) TRACE_category(const char *category);
XBT_PUBLIC(void) TRACE_category_with_color (const char *category, const char *color);
XBT_PUBLIC(void) TRACE_smpi_set_category(const char *category);
XBT_PUBLIC(void) TRACE_sd_set_task_category(SD_task_t task, const char *category);

/*
 * Functions to manage tracing marks (used for trace comparison experiments)
 */
XBT_PUBLIC(void) TRACE_declare_mark(const char *mark_type);
XBT_PUBLIC(void) TRACE_mark(const char *mark_type, const char *mark_value);

/*
 * Function used by graphicator (transform a SimGrid platform
 * file in a graphviz dot file with the network topology)
 */
XBT_PUBLIC(int) TRACE_platform_graph_export_graphviz (const char *filename);

/*
 * User-variables related functions
 */
/* for host variables */
XBT_PUBLIC(void) TRACE_host_variable_declare (const char *variable);
XBT_PUBLIC(void) TRACE_host_variable_declare_with_color (const char *variable, const char *color);
XBT_PUBLIC(void) TRACE_host_variable_set (const char *host, const char *variable, double value);
XBT_PUBLIC(void) TRACE_host_variable_add (const char *host, const char *variable, double value);
XBT_PUBLIC(void) TRACE_host_variable_sub (const char *host, const char *variable, double value);
XBT_PUBLIC(void) TRACE_host_variable_set_with_time (double time, const char *host, const char *variable, double value);
XBT_PUBLIC(void) TRACE_host_variable_add_with_time (double time, const char *host, const char *variable, double value);
XBT_PUBLIC(void) TRACE_host_variable_sub_with_time (double time, const char *host, const char *variable, double value);

/* for link variables */
XBT_PUBLIC(void) TRACE_link_variable_declare (const char *var);
XBT_PUBLIC(void) TRACE_link_variable_declare_with_color (const char *var, const char *color);
XBT_PUBLIC(void) TRACE_link_variable_set (const char *link, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_variable_add (const char *link, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_variable_sub (const char *link, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_variable_set_with_time (double time, const char *link, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_variable_add_with_time (double time, const char *link, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_variable_sub_with_time (double time, const char *link, const char *variable, double value);

/* for link variables, but with src and dst used for get_route */
XBT_PUBLIC(void) TRACE_link_srcdst_variable_set (const char *src, const char *dst, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_srcdst_variable_add (const char *src, const char *dst, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_srcdst_variable_sub (const char *src, const char *dst, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_srcdst_variable_set_with_time (double time, const char *src, const char *dst, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_srcdst_variable_add_with_time (double time, const char *src, const char *dst, const char *variable, double value);
XBT_PUBLIC(void) TRACE_link_srcdst_variable_sub_with_time (double time, const char *src, const char *dst, const char *variable, double value);

#else                           /* HAVE_TRACING */

#define TRACE_category(category)
#define TRACE_category_with_color(category,color)
#define TRACE_msg_set_task_category(task,category)
#define TRACE_smpi_set_category(category)
#define TRACE_sd_set_task_category(task,category)

#define TRACE_declare_mark(mark_type)
#define TRACE_mark(mark_type,mark_value)

#define TRACE_platform_graph_export_graphviz(filename)

#define TRACE_host_variable_declare(var)
#define TRACE_host_variable_declare_with_color(var,color)
#define TRACE_host_variable_set(host,var,value)
#define TRACE_host_variable_add(host,var,value)
#define TRACE_host_variable_sub(host,var,value)
#define TRACE_host_variable_set_with_time(time,host,var,value)
#define TRACE_host_variable_add_with_time(time,host,var,value)
#define TRACE_host_variable_sub_with_time(time,host,var,value)

#define TRACE_link_variable_declare(var)
#define TRACE_link_variable_declare_with_color(var,color)
#define TRACE_link_variable_set(link,var,value)
#define TRACE_link_variable_add(link,var,value)
#define TRACE_link_variable_sub(link,var,value)
#define TRACE_link_variable_set_with_time(time,link,var,value)
#define TRACE_link_variable_add_with_time(time,link,var,value)
#define TRACE_link_variable_sub_with_time(time,link,var,value)

#define TRACE_link_srcdst_variable_set(src,dst,var,value)
#define TRACE_link_srcdst_variable_add(src,dst,var,value)
#define TRACE_link_srcdst_variable_sub(src,dst,var,value)
#define TRACE_link_srcdst_variable_set_with_time(time,src,dst,var,value)
#define TRACE_link_srcdst_variable_add_with_time(time,src,dst,var,value)
#define TRACE_link_srcdst_variable_sub_with_time(time,src,dst,var,value)

#endif                          /* HAVE_TRACING */

#endif                          /* INSTR_H_ */
