/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_HPP
#define INSTR_PRIVATE_HPP

#include <xbt/base.h>

#include "instr/instr_interface.h"
#include "simgrid/instr.h"
#include "simgrid_config.h"
#include "src/instr/instr_paje_containers.hpp"
#include "src/instr/instr_paje_events.hpp"
#include "src/instr/instr_paje_types.hpp"
#include "src/instr/instr_paje_values.hpp"
#include "src/internal_config.h"
#include "xbt/graph.h"
#include <iomanip> /** std::setprecision **/
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#ifdef WIN32
#include <direct.h> // _mkdir
/* Need to define function drand48 for Windows */
/* FIXME: use _drand48() defined in src/surf/random_mgr.c instead */
#define drand48() (rand() / (RAND_MAX + 1.0))
#endif

typedef simgrid::instr::Container* container_t;

extern "C" {

extern XBT_PRIVATE std::set<std::string> created_categories;
extern XBT_PRIVATE std::set<std::string> declared_marks;
extern XBT_PRIVATE std::set<std::string> user_host_variables;
extern XBT_PRIVATE std::set<std::string> user_vm_variables;
extern XBT_PRIVATE std::set<std::string> user_link_variables;
extern XBT_PRIVATE double TRACE_last_timestamp_to_dump;

/* instr_paje_header.c */
XBT_PRIVATE void TRACE_header(bool basic, int size);

/* from paje.c */
XBT_PRIVATE void TRACE_paje_start();
XBT_PRIVATE void TRACE_paje_end();

/* from instr_config.c */
XBT_PRIVATE bool TRACE_needs_platform();
XBT_PRIVATE bool TRACE_is_enabled();
XBT_PRIVATE bool TRACE_platform();
XBT_PRIVATE bool TRACE_platform_topology();
XBT_PRIVATE bool TRACE_is_configured();
XBT_PRIVATE bool TRACE_categorized();
XBT_PRIVATE bool TRACE_uncategorized();
XBT_PRIVATE bool TRACE_msg_process_is_enabled();
XBT_PRIVATE bool TRACE_msg_vm_is_enabled();
XBT_PRIVATE bool TRACE_buffer();
XBT_PRIVATE bool TRACE_disable_link();
XBT_PRIVATE bool TRACE_disable_speed();
XBT_PRIVATE bool TRACE_onelink_only();
XBT_PRIVATE bool TRACE_disable_destroy();
XBT_PRIVATE bool TRACE_basic();
XBT_PRIVATE bool TRACE_display_sizes();
XBT_PRIVATE int TRACE_precision();
XBT_PRIVATE void TRACE_generate_viva_uncat_conf();
XBT_PRIVATE void TRACE_generate_viva_cat_conf();
XBT_PRIVATE void instr_pause_tracing();
XBT_PRIVATE void instr_resume_tracing();

/* Public functions used in SMPI */
XBT_PUBLIC(bool) TRACE_smpi_is_enabled();
XBT_PUBLIC(bool) TRACE_smpi_is_grouped();
XBT_PUBLIC(bool) TRACE_smpi_is_computing();
XBT_PUBLIC(bool) TRACE_smpi_is_sleeping();
XBT_PUBLIC(bool) TRACE_smpi_view_internals();

/* from resource_utilization.c */
XBT_PRIVATE void TRACE_surf_host_set_utilization(const char* resource, const char* category, double value, double now,
                                                 double delta);
XBT_PRIVATE void TRACE_surf_link_set_utilization(const char* resource, const char* category, double value, double now,
                                                 double delta);
XBT_PUBLIC(void) TRACE_surf_resource_utilization_alloc();

/* instr_paje.c */
extern XBT_PRIVATE std::set<std::string> trivaNodeTypes;
extern XBT_PRIVATE std::set<std::string> trivaEdgeTypes;
XBT_PRIVATE long long int instr_new_paje_id();
void instr_new_variable_type(std::string new_typename, std::string color);
void instr_new_user_variable_type(std::string father_type, std::string new_typename, std::string color);
void instr_new_user_state_type(std::string father_type, std::string new_typename);
void instr_new_value_for_user_state_type(std::string new_typename, const char* value, std::string color);

/* instr_config.c */
XBT_PRIVATE void TRACE_TI_start();
XBT_PRIVATE void TRACE_TI_end();

XBT_PRIVATE void TRACE_paje_dump_buffer(bool force);
XBT_PRIVATE void dump_comment_file(std::string filename);
XBT_PRIVATE void dump_comment(std::string comment);

enum e_caller_type {
  TRACING_INIT,
  TRACING_FINALIZE,
  TRACING_COMM_SIZE,
  TRACING_COMM_SPLIT,
  TRACING_COMM_DUP,
  TRACING_SEND,
  TRACING_ISEND,
  TRACING_SSEND,
  TRACING_ISSEND,
  TRACING_RECV,
  TRACING_IRECV,
  TRACING_SENDRECV,
  TRACING_TEST,
  TRACING_WAIT,
  TRACING_WAITALL,
  TRACING_WAITANY,
  TRACING_BARRIER,
  TRACING_BCAST,
  TRACING_REDUCE,
  TRACING_ALLREDUCE,
  TRACING_ALLTOALL,
  TRACING_ALLTOALLV,
  TRACING_GATHER,
  TRACING_GATHERV,
  TRACING_SCATTER,
  TRACING_SCATTERV,
  TRACING_ALLGATHER,
  TRACING_ALLGATHERV,
  TRACING_REDUCE_SCATTER,
  TRACING_COMPUTING,
  TRACING_SLEEPING,
  TRACING_SCAN,
  TRACING_EXSCAN
};

struct s_instr_extra_data_t {
  e_caller_type type;
  int send_size;
  int recv_size;
  double comp_size;
  double sleep_duration;
  int src;
  int dst;
  int root;
  const char* datatype1;
  const char* datatype2;
  int* sendcounts;
  int* recvcounts;
  int num_processes;
};

typedef s_instr_extra_data_t* instr_extra_data;

/* Format of TRACING output.
 *   - paje is the regular format, that we all know
 *   - TI is a trick to reuse the tracing functions to generate a time independent trace during the execution. Such
 *     trace can easily be replayed with smpi_replay afterward. This trick should be removed and replaced by some code
 *     using the signal that we will create to cleanup the TRACING
 */
enum instr_fmt_type_t { instr_fmt_paje, instr_fmt_TI };
extern instr_fmt_type_t instr_fmt_type;
}
XBT_PRIVATE std::string TRACE_get_comment();
XBT_PRIVATE std::string TRACE_get_comment_file();
XBT_PRIVATE std::string TRACE_get_filename();
XBT_PRIVATE std::string TRACE_get_viva_uncat_conf();
XBT_PRIVATE std::string TRACE_get_viva_cat_conf();

#endif
