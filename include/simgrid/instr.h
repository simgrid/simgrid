/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_H_
#define INSTR_H_

#include <simgrid/msg.h>

#ifdef __cplusplus
#include <set>
#include <string>

namespace simgrid {
namespace instr {
void declare_host_variable(const std::string& variable, const std::string& color = std::string(""));
void set_host_variable(const s4u::Host* host, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void add_host_variable(const s4u::Host* host, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void sub_host_variable(const s4u::Host* host, const std::string& variable, double value,
                       double time = simgrid_get_clock());
const std::set<std::string, std::less<>>& get_host_variables();

void declare_link_variable(const std::string& variable, const std::string& color = std::string(""));
void set_link_variable(const s4u::Link* link, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void set_link_variable(const s4u::Host* src, const s4u::Host* dst, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void add_link_variable(const s4u::Link* link, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void add_link_variable(const s4u::Host* src, const s4u::Host* dst, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void sub_link_variable(const s4u::Link* link, const std::string& variable, double value,
                       double time = simgrid_get_clock());
void sub_link_variable(const s4u::Host* src, const s4u::Host* dst, const std::string& variable, double value,
                       double time = simgrid_get_clock());
const std::set<std::string, std::less<>>& get_link_variables();

void declare_mark(const std::string& mark_type);
void declare_mark_value(const std::string& mark_type, const std::string& mark_value,
                        const std::string& mark_color = std::string("1 1 1"));
void mark(const std::string& mark_type, const std::string& mark_value);
const std::set<std::string, std::less<>>& get_marks();

void declare_tracing_category(const std::string& name, const std::string& color = "");
const std::set<std::string, std::less<>>& get_tracing_categories();
} // namespace instr
} // namespace simgrid

#endif
SG_BEGIN_DECL

/* Functions to manage tracing categories */
XBT_PUBLIC void TRACE_category(const char* category);
XBT_PUBLIC void TRACE_category_with_color(const char* category, const char* color);
XBT_PUBLIC xbt_dynar_t TRACE_get_categories();
XBT_PUBLIC void TRACE_smpi_set_category(const char* category);

/*  Functions to manage tracing marks (used for trace comparison experiments) */
XBT_PUBLIC void TRACE_declare_mark(const char* mark_type);
XBT_PUBLIC void TRACE_declare_mark_value_with_color(const char* mark_type, const char* mark_value,
                                                    const char* mark_color);
XBT_PUBLIC void TRACE_declare_mark_value(const char* mark_type, const char* mark_value);
XBT_PUBLIC void TRACE_mark(const char* mark_type, const char* mark_value);
XBT_PUBLIC xbt_dynar_t TRACE_get_marks();

/* Function used by graphicator (transform a SimGrid platform file in a graphviz dot file with the network topology) */
XBT_PUBLIC int TRACE_platform_graph_export_graphviz(const char* filename);

/* User-variables related functions*/
/* for VM variables */
XBT_PUBLIC void TRACE_vm_variable_declare(const char* variable);
XBT_PUBLIC void TRACE_vm_variable_declare_with_color(const char* variable, const char* color);
XBT_PUBLIC void TRACE_vm_variable_set(const char* vm, const char* variable, double value);
XBT_PUBLIC void TRACE_vm_variable_add(const char* vm, const char* variable, double value);
XBT_PUBLIC void TRACE_vm_variable_sub(const char* vm, const char* variable, double value);
XBT_PUBLIC void TRACE_vm_variable_set_with_time(double time, const char* vm, const char* variable, double value);
XBT_PUBLIC void TRACE_vm_variable_add_with_time(double time, const char* vm, const char* variable, double value);
XBT_PUBLIC void TRACE_vm_variable_sub_with_time(double time, const char* vm, const char* variable, double value);

/* for host variables */
XBT_PUBLIC void TRACE_host_variable_declare(const char* variable);
XBT_PUBLIC void TRACE_host_variable_declare_with_color(const char* variable, const char* color);
XBT_PUBLIC void TRACE_host_variable_set(const char* host, const char* variable, double value);
XBT_PUBLIC void TRACE_host_variable_add(const char* host, const char* variable, double value);
XBT_PUBLIC void TRACE_host_variable_sub(const char* host, const char* variable, double value);
XBT_PUBLIC void TRACE_host_variable_set_with_time(double time, const char* host, const char* variable, double value);
XBT_PUBLIC void TRACE_host_variable_add_with_time(double time, const char* host, const char* variable, double value);
XBT_PUBLIC void TRACE_host_variable_sub_with_time(double time, const char* host, const char* variable, double value);
XBT_PUBLIC xbt_dynar_t TRACE_get_host_variables();

/* for link variables */
XBT_PUBLIC void TRACE_link_variable_declare(const char* var);
XBT_PUBLIC void TRACE_link_variable_declare_with_color(const char* var, const char* color);
XBT_PUBLIC void TRACE_link_variable_set(const char* link, const char* variable, double value);
XBT_PUBLIC void TRACE_link_variable_add(const char* link, const char* variable, double value);
XBT_PUBLIC void TRACE_link_variable_sub(const char* link, const char* variable, double value);
XBT_PUBLIC void TRACE_link_variable_set_with_time(double time, const char* link, const char* variable, double value);
XBT_PUBLIC void TRACE_link_variable_add_with_time(double time, const char* link, const char* variable, double value);
XBT_PUBLIC void TRACE_link_variable_sub_with_time(double time, const char* link, const char* variable, double value);

/* for link variables, but with src and dst used for get_route */
XBT_PUBLIC void TRACE_link_srcdst_variable_set(const char* src, const char* dst, const char* variable, double value);
XBT_PUBLIC void TRACE_link_srcdst_variable_add(const char* src, const char* dst, const char* variable, double value);
XBT_PUBLIC void TRACE_link_srcdst_variable_sub(const char* src, const char* dst, const char* variable, double value);
XBT_PUBLIC void TRACE_link_srcdst_variable_set_with_time(double time, const char* src, const char* dst,
                                                         const char* variable, double value);
XBT_PUBLIC void TRACE_link_srcdst_variable_add_with_time(double time, const char* src, const char* dst,
                                                         const char* variable, double value);
XBT_PUBLIC void TRACE_link_srcdst_variable_sub_with_time(double time, const char* src, const char* dst,
                                                         const char* variable, double value);
XBT_PUBLIC xbt_dynar_t TRACE_get_link_variables();
XBT_PUBLIC void TRACE_host_state_declare(const char* state);
XBT_PUBLIC void TRACE_host_state_declare_value(const char* state, const char* value, const char* color);
XBT_PUBLIC void TRACE_host_set_state(const char* host, const char* state, const char* value);
XBT_PUBLIC void TRACE_host_push_state(const char* host, const char* state, const char* value);
XBT_PUBLIC void TRACE_host_pop_state(const char* host, const char* state);

SG_END_DECL

#endif                          /* INSTR_H_ */
