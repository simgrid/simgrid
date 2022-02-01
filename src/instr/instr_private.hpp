/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_PRIVATE_HPP
#define INSTR_PRIVATE_HPP

#include <xbt/base.h>

#include "simgrid/instr.h"
#include "simgrid/s4u/Actor.hpp"
#include "src/instr/instr_paje_containers.hpp"
#include "src/instr/instr_paje_events.hpp"
#include "src/instr/instr_paje_types.hpp"
#include "src/instr/instr_paje_values.hpp"

#include <fstream>
#include <iomanip> /** std::setprecision **/
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>

namespace simgrid {
namespace instr {
namespace paje {

void dump_generator_version();
void dump_comment_file(const std::string& filename);
void dump_header(bool basic, bool display_sizes);
} // namespace paje

/* Format of TRACING output.
 *   - paje is the regular format, that we all know
 *   - TI is a trick to reuse the tracing functions to generate a time independent trace during the execution. Such
 *     trace can easily be replayed with smpi_replay afterward. This trick should be removed and replaced by some code
 *     using the signal that we will create to cleanup the TRACING
 */
enum class TraceFormat { Paje, /*TimeIndependent*/ Ti };
extern TraceFormat trace_format;
extern int trace_precision;
extern double last_timestamp_to_dump;

void init();
void define_callbacks();

void resource_set_utilization(const char* type, const char* name, const char* resource, const std::string& category,
                              double value, double now, double delta);
void dump_buffer(bool force);

class TIData {
  std::string name_;
  double amount_ = 0;

public:
  explicit TIData(const std::string& name) : name_(name){};
  explicit TIData(const std::string& name, double amount) : name_(name), amount_(amount){};

  virtual ~TIData() = default;

  const std::string& get_name() const { return name_; }
  double get_amount() const { return amount_; }
  virtual std::string print()        = 0;
  virtual std::string display_size() = 0;
};

// NoOpTI: init, finalize, test, wait, barrier
class NoOpTIData : public TIData {
  explicit NoOpTIData(const std::string&, double); // disallow this constructor inherited from TIData

public:
  using TIData::TIData;
  std::string print() override { return get_name(); }
  std::string display_size() override { return "NA"; }
};

// CPuTI: compute, sleep (+ waitAny and waitall out of laziness)
class CpuTIData : public TIData {
  explicit CpuTIData(const std::string&); // disallow this constructor inherited from TIData

public:
  using TIData::TIData;
  std::string print() override
  {
    std::stringstream stream;
    stream << get_name() << " " << get_amount();
    return stream.str();
  }
  std::string display_size() override { return std::to_string(get_amount()); }
};

// Pt2PtTI: send, isend, ssend, issend, recv, irecv
class Pt2PtTIData : public TIData {
  int endpoint_;
  size_t size_;
  std::string type_;
  int tag_ = 0;

public:
  Pt2PtTIData(const std::string& name, int endpoint, size_t size, const std::string& datatype)
      : TIData(name), endpoint_(endpoint), size_(size), type_(datatype){};
  Pt2PtTIData(const std::string& name, int endpoint, size_t size, int tag, const std::string& datatype)
      : TIData(name), endpoint_(endpoint), size_(size), type_(datatype), tag_(tag){};

  std::string print() override
  {
    std::stringstream stream;
    stream << get_name() << " " << endpoint_ << " " << tag_ << " " << size_ << " " << type_;
    return stream.str();
  }
  std::string display_size() override { return std::to_string(size_); }
};

// CollTI: bcast, reduce, allreduce, gather, scatter, allgather, alltoall
class CollTIData : public TIData {
  int root_;
  size_t send_size_;
  size_t recv_size_;
  std::string send_type_;
  std::string recv_type_;

public:
  CollTIData(const std::string& name, int root, double amount, size_t send_size, size_t recv_size,
             const std::string& send_type, const std::string& recv_type)
      : TIData(name, amount)
      , root_(root)
      , send_size_(send_size)
      , recv_size_(recv_size)
      , send_type_(send_type)
      , recv_type_(recv_type){};

  std::string print() override
  {
    std::stringstream stream;
    stream << get_name() << " " << send_size_ << " ";
    if (recv_size_ > 0)
      stream << recv_size_ << " ";
    if (get_amount() >= 0.0)
      stream << get_amount() << " ";
    if (root_ > 0 || (root_ == 0 && not send_type_.empty()))
      stream << root_ << " ";
    stream << send_type_ << " " << recv_type_;

    return stream.str();
  }
  std::string display_size() override { return std::to_string(send_size_); }
};

// VarCollTI: gatherv, scatterv, allgatherv, alltoallv (+ reducescatter out of laziness)
class VarCollTIData : public TIData {
  int root_;
  long int send_size_;
  std::shared_ptr<std::vector<int>> sendcounts_;
  long int recv_size_;
  std::shared_ptr<std::vector<int>> recvcounts_;
  std::string send_type_;
  std::string recv_type_;

public:
  VarCollTIData(const std::string& name, int root, long int send_size, std::shared_ptr<std::vector<int>> sendcounts,
                long int recv_size, std::shared_ptr<std::vector<int>> recvcounts, const std::string& send_type,
                const std::string& recv_type)
      : TIData(name)
      , root_(root)
      , send_size_(send_size)
      , sendcounts_(sendcounts)
      , recv_size_(recv_size)
      , recvcounts_(recvcounts)
      , send_type_(send_type)
      , recv_type_(recv_type){};

  std::string print() override
  {
    std::stringstream stream;
    stream << get_name() << " ";
    if (send_size_ > -1)
      stream << send_size_ << " ";
    if (sendcounts_ != nullptr)
      for (auto count : *sendcounts_)
        stream << count << " ";
    if (recv_size_ > -1)
      stream << recv_size_ << " ";
    if (recvcounts_ != nullptr)
      for (auto count : *recvcounts_)
        stream << count << " ";
    if (root_ > 0 || (root_ == 0 && not send_type_.empty()))
      stream << root_ << " ";
    stream << send_type_ << " " << recv_type_;

    return stream.str();
  }
  std::string display_size() override { return std::to_string(send_size_ > 0 ? send_size_ : recv_size_); }
};

/**
 * If we want to wait for a request of asynchronous communication, we need to be able
 * to identify this request. We do this by searching for a request identified by (src, dest, tag).
 */
class WaitTIData : public TIData {
  int src_;
  int dest_;
  int tag_;

public:
  WaitTIData(const std::string& name, int src, int dest, int tag) : TIData(name), src_(src), dest_(dest), tag_(tag){};

  std::string print() override
  {
    std::stringstream stream;
    stream << get_name() << " " << src_ << " " << dest_ << " " << tag_;
    return stream.str();
  }
  std::string display_size() override { return "NA"; }
};

class AmpiMigrateTIData : public TIData {
  size_t memory_consumption_;

public:
  explicit AmpiMigrateTIData(size_t memory_conso) : TIData("migrate"), memory_consumption_(memory_conso){};

  std::string print() override
  {
    std::stringstream stream;
    stream << get_name() << " " << memory_consumption_;
    return stream.str();
  }
  std::string display_size() override { return "NA"; }
};
} // namespace instr
} // namespace simgrid

XBT_PRIVATE std::string instr_pid(simgrid::s4u::Actor const& proc);

extern XBT_PRIVATE std::set<std::string, std::less<>> created_categories;
extern XBT_PRIVATE std::set<std::string, std::less<>> declared_marks;
extern XBT_PRIVATE std::set<std::string, std::less<>> user_host_variables;
extern XBT_PRIVATE std::set<std::string, std::less<>> user_vm_variables;
extern XBT_PRIVATE std::set<std::string, std::less<>> user_link_variables;

/* from instr_config.c */
XBT_PRIVATE bool TRACE_needs_platform();
XBT_PRIVATE bool TRACE_is_enabled();
XBT_PRIVATE bool TRACE_platform();
XBT_PRIVATE bool TRACE_platform_topology();
XBT_PRIVATE bool TRACE_categorized();
XBT_PRIVATE bool TRACE_uncategorized();
XBT_PRIVATE bool TRACE_actor_is_enabled();
XBT_PRIVATE bool TRACE_vm_is_enabled();
XBT_PRIVATE bool TRACE_disable_link();
XBT_PRIVATE bool TRACE_disable_speed();
XBT_PRIVATE bool TRACE_display_sizes();

/* Public functions used in SMPI */
XBT_PUBLIC bool TRACE_smpi_is_enabled();
XBT_PUBLIC bool TRACE_smpi_is_grouped();
XBT_PUBLIC bool TRACE_smpi_is_computing();
XBT_PUBLIC bool TRACE_smpi_is_sleeping();
XBT_PUBLIC bool TRACE_smpi_view_internals();

/* instr_paje.c */
void instr_new_variable_type(const std::string& new_typename, const std::string& color);
void instr_new_user_variable_type(const std::string& parent_type, const std::string& new_typename,
                                  const std::string& color);
void instr_new_user_state_type(const std::string& parent_type, const std::string& new_typename);
void instr_new_value_for_user_state_type(const std::string& new_typename, const char* value, const std::string& color);

XBT_PRIVATE void TRACE_help();

#endif
