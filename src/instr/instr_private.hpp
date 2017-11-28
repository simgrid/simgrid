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

namespace simgrid {
namespace instr {

class TIData {
  std::string name_;
  double amount_ = 0;

public:
  int endpoint                 = 0;
  int send_size                = 0;
  std::vector<int>* sendcounts = nullptr;
  int recv_size                = 0;
  std::vector<int>* recvcounts = nullptr;
  std::string send_type        = "";
  std::string recv_type        = "";

  // NoOpTI: init, finalize, test, wait, barrier
  explicit TIData(std::string name) : name_(name){};
  // CPuTI: compute, sleep (+ waitAny and waitAll out of laziness)
  explicit TIData(std::string name, double amount) : name_(name), amount_(amount){};
  // Pt2PtTI: send, isend, sssend, issend, recv, irecv
  explicit TIData(std::string name, int endpoint, int size, std::string datatype)
      : name_(name), endpoint(endpoint), send_size(size), send_type(datatype){};
  // CollTI: bcast, reduce, allReduce, gather, scatter, allGather, allToAll
  explicit TIData(std::string name, int root, double amount, int send_size, int recv_size, std::string send_type,
                  std::string recv_type)
      : name_(name)
      , amount_(amount)
      , endpoint(root)
      , send_size(send_size)
      , recv_size(recv_size)
      , send_type(send_type)
      , recv_type(recv_type){};
  // VarCollTI: gatherV, scatterV, allGatherV, allToAllV (+ reduceScatter out of laziness)
  explicit TIData(std::string name, int root, int send_size, std::vector<int>* sendcounts, int recv_size,
                  std::vector<int>* recvcounts, std::string send_type, std::string recv_type)
      : name_(name)
      , endpoint(root)
      , send_size(send_size)
      , sendcounts(sendcounts)
      , recv_size(recv_size)
      , recvcounts(recvcounts)
      , send_type(send_type)
      , recv_type(recv_type){};

  virtual ~TIData()
  {
    delete sendcounts;
    delete recvcounts;
  }

  std::string getName() { return name_; }
  double getAmount() { return amount_; }
  virtual std::string print()        = 0;
  virtual std::string display_size() = 0;
};

class NoOpTIData : public TIData {
public:
  explicit NoOpTIData(std::string name) : TIData(name){};
  std::string print() override { return getName(); }
  std::string display_size() override { return ""; }
};

class CpuTIData : public TIData {
public:
  explicit CpuTIData(std::string name, double amount) : TIData(name, amount){};
  std::string print() override
  {
    std::stringstream stream;
    stream << getName() << " " << getAmount();
    return stream.str();
  }
  std::string display_size() override { return std::to_string(getAmount()); }
};

class Pt2PtTIData : public TIData {
public:
  explicit Pt2PtTIData(std::string name, int endpoint, int size, std::string datatype)
      : TIData(name, endpoint, size, datatype){};
  std::string print() override
  {
    std::stringstream stream;
    stream << getName() << " ";
    if (endpoint >= 0)
      stream << endpoint << " ";
    stream << send_size << " " << send_type;
    return stream.str();
  }
  std::string display_size() override { return std::to_string(send_size); }
};

class CollTIData : public TIData {
public:
  explicit CollTIData(std::string name, int root, double amount, int send_size, int recv_size, std::string send_type,
                      std::string recv_type)
      : TIData(name, root, amount, send_size, recv_size, send_type, recv_type){};
  std::string print() override
  {
    std::stringstream stream;
    stream << getName() << " " << send_size << " ";
    if (recv_size >= 0)
      stream << recv_size << " ";
    if (getAmount() >= 0.0)
      stream << getAmount() << " ";
    if (endpoint > 0 || (endpoint == 0 && not send_type.empty()))
      stream << endpoint << " ";
    stream << send_type << " " << recv_type;

    return stream.str();
  }
  std::string display_size() override { return std::to_string(send_size); }
};

class VarCollTIData : public TIData {
public:
  explicit VarCollTIData(std::string name, int root, int send_size, std::vector<int>* sendcounts, int recv_size,
                         std::vector<int>* recvcounts, std::string send_type, std::string recv_type)
      : TIData(name, root, send_size, sendcounts, recv_size, recvcounts, send_type, recv_type){};
  std::string print() override
  {
    std::stringstream stream;
    stream << getName() << " ";
    if (send_size >= 0)
      stream << send_size << " ";
    if (sendcounts != nullptr)
      for (auto count : *sendcounts)
        stream << count << " ";
    if (recv_size >= 0)
      stream << recv_size << " ";
    if (recvcounts != nullptr)
      for (auto count : *recvcounts)
        stream << count << " ";
    if (endpoint > 0 || (endpoint == 0 && not send_type.empty()))
      stream << endpoint << " ";
    stream << send_type << " " << recv_type;

    return stream.str();
  }
  std::string display_size() override { return std::to_string(send_size > 0 ? send_size : recv_size); }
};
}
}

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

#endif
