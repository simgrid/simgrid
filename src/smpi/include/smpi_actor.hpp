/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_ACTOR_HPP
#define SMPI_ACTOR_HPP

#include "private.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/instr/instr_smpi.hpp"

namespace simgrid {
namespace smpi {

class ActorExt {
  double simulated_ = 0 /* Used to time with simulated_start/elapsed */;
  s4u::Mailbox* mailbox_;
  s4u::Mailbox* mailbox_small_;
  s4u::MutexPtr mailboxes_mutex_;
  xbt_os_timer_t timer_;
  MPI_Comm comm_self_   = MPI_COMM_NULL;
  MPI_Comm comm_intra_  = MPI_COMM_NULL;
  MPI_Comm* comm_world_ = nullptr;
  SmpiProcessState state_;
  int sampling_ = 0; /* inside an SMPI_SAMPLE_ block? */
  std::string instance_id_;
  bool replaying_ = false; /* is the process replaying a trace */
  smpi_trace_call_location_t trace_call_loc_;
  s4u::Actor* actor_                             = nullptr;
  smpi_privatization_region_t privatized_region_ = nullptr;
#ifdef __linux__
  int optind_                                     = 0; /*for getopt replacement */
#else
  int optind_                                     = 1; /*for getopt replacement */
#endif
  std::string tracing_category_                  = "";
  MPI_Info info_env_;
  void* bsend_buffer_    = nullptr;
  int bsend_buffer_size_ = 0;

#if HAVE_PAPI
  /** Contains hardware data as read by PAPI **/
  int papi_event_set_;
  papi_counter_t papi_counter_data_;
#endif

public:
  static simgrid::xbt::Extension<simgrid::s4u::Actor, ActorExt> EXTENSION_ID;

  explicit ActorExt(s4u::Actor* actor);
  ActorExt(const ActorExt&) = delete;
  ActorExt& operator=(const ActorExt&) = delete;
  ~ActorExt();
  void finalize();
  int finalized() const;
  int initializing() const;
  int initialized() const;
  int finalizing() const;
  void mark_as_initialized();
  void mark_as_finalizing();
  void set_replaying(bool value);
  bool replaying() const;
  std::string get_instance_id() const { return instance_id_;}
  void set_tracing_category(const std::string& category) { tracing_category_ = category; }
  const std::string& get_tracing_category() const { return tracing_category_; }
  smpi_trace_call_location_t* call_location();
  void set_privatized_region(smpi_privatization_region_t region);
  smpi_privatization_region_t privatized_region() const;
  s4u::Mailbox* mailbox() const { return mailbox_; }
  s4u::Mailbox* mailbox_small() const { return mailbox_small_; }
  s4u::MutexPtr mailboxes_mutex() const;
#if HAVE_PAPI
  int papi_event_set() const;
  papi_counter_t& papi_counters();
#endif
  xbt_os_timer_t timer();
  void simulated_start();
  double simulated_elapsed() const;
  MPI_Comm comm_world() const;
  bool comm_self_is_set() const { return (comm_self_ != MPI_COMM_NULL); };
  MPI_Comm comm_self();
  MPI_Comm comm_intra();
  void set_comm_intra(MPI_Comm comm);
  void set_sampling(int s);
  int sampling() const;
  static void init();
  s4u::ActorPtr get_actor();
  int get_optind() const;
  void set_optind(int optind);
  MPI_Info info_env();
  void bsend_buffer(void** buf, int* size);
  int set_bsend_buffer(void* buf, int size);
};

} // namespace smpi
} // namespace simgrid

#endif
