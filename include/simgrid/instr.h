/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_H_
#define INSTR_H_

#include <simgrid/engine.h>

#ifdef __cplusplus
#include <set>
#include <string>

namespace simgrid::instr {
/* User-variables related functions*/
/* for host variables */
XBT_PUBLIC void declare_host_variable(const std::string& variable, const std::string& color = "");
XBT_PUBLIC void set_host_variable(const std::string& host, const std::string& variable, double value,
                                  double time = simgrid_get_clock());
XBT_PUBLIC void add_host_variable(const std::string& host, const std::string& variable, double value,
                                  double time = simgrid_get_clock());
XBT_PUBLIC void sub_host_variable(const std::string& host, const std::string& variable, double value,
                                  double time = simgrid_get_clock());
XBT_PUBLIC const std::set<std::string, std::less<>>& get_host_variables();

/* for link variables */
XBT_PUBLIC void declare_link_variable(const std::string& variable, const std::string& color = "");
XBT_PUBLIC void set_link_variable(const std::string& link, const std::string& variable, double value,
                                  double time = simgrid_get_clock());
XBT_PUBLIC void add_link_variable(const std::string& link, const std::string& variable, double value,
                                  double time = simgrid_get_clock());
XBT_PUBLIC void sub_link_variable(const std::string& link, const std::string& variable, double value,
                                  double time = simgrid_get_clock());
/* for link variables, but with src and dst used for get_route */
XBT_PUBLIC void set_link_variable(const std::string& src, const std::string& dst, const std::string& variable,
                                  double value, double time = simgrid_get_clock());
XBT_PUBLIC void add_link_variable(const std::string& src, const std::string& dst, const std::string& variable,
                                  double value, double time = simgrid_get_clock());
XBT_PUBLIC void sub_link_variable(const std::string& src, const std::string& dst, const std::string& variable,
                                  double value, double time = simgrid_get_clock());
XBT_PUBLIC const std::set<std::string, std::less<>>& get_link_variables();

/* for VM variables */
XBT_PUBLIC void declare_vm_variable(const std::string& variable, const std::string& color = "");
XBT_PUBLIC void set_vm_variable(const std::string& vm, const std::string& variable, double value,
                                double time = simgrid_get_clock());
XBT_PUBLIC void add_vm_variable(const std::string& vm, const std::string& variable, double value,
                                double time = simgrid_get_clock());
XBT_PUBLIC void sub_vm_variable(const std::string& vm, const std::string& variable, double value,
                                double time = simgrid_get_clock());
XBT_PUBLIC const std::set<std::string, std::less<>>& get_vm_variables();

/*  Functions to manage tracing marks (used for trace comparison experiments) */
XBT_PUBLIC void declare_mark(const std::string& mark_type);
XBT_PUBLIC void declare_mark_value(const std::string& mark_type, const std::string& mark_value,
                                   const std::string& mark_color = "1 1 1");
XBT_PUBLIC void mark(const std::string& mark_type, const std::string& mark_value);
XBT_PUBLIC const std::set<std::string, std::less<>>& get_marks();

XBT_PUBLIC void declare_tracing_category(const std::string& name, const std::string& color = "");
XBT_PUBLIC const std::set<std::string, std::less<>>& get_tracing_categories();

/* Function used by graphicator (transform a SimGrid platform file in a graphviz dot file with the network topology) */
XBT_PUBLIC void platform_graph_export_graphviz(const std::string& output_filename);
/* Function used by graphicator (transform a SimGrid platform file in a CSV file with the network topology) */
XBT_PUBLIC void platform_graph_export_csv(const std::string& output_filename);
} // namespace simgrid::instr

#endif
SG_BEGIN_DECL

/* Functions to manage tracing categories */
XBT_PUBLIC void TRACE_smpi_set_category(const char* category);

XBT_PUBLIC void TRACE_host_state_declare(const char* state);
XBT_PUBLIC void TRACE_host_state_declare_value(const char* state, const char* value, const char* color);
XBT_PUBLIC void TRACE_host_set_state(const char* host, const char* state, const char* value);
XBT_PUBLIC void TRACE_host_push_state(const char* host, const char* state, const char* value);
XBT_PUBLIC void TRACE_host_pop_state(const char* host, const char* state);

SG_END_DECL

#endif /* INSTR_H_ */
