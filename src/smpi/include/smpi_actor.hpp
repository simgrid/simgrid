/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_ACTOR_HPP
#define SMPI_ACTOR_HPP

#include "private.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/instr/instr_smpi.hpp"
#include "xbt/synchro.h"

namespace simgrid {
namespace smpi {

class ActorExt {
private:
  double simulated_ = 0 /* Used to time with simulated_start/elapsed */;
  simgrid::s4u::MailboxPtr mailbox_;
  simgrid::s4u::MailboxPtr mailbox_small_;
  xbt_mutex_t mailboxes_mutex_;
  xbt_os_timer_t timer_;
  MPI_Comm comm_self_   = MPI_COMM_NULL;
  MPI_Comm comm_intra_  = MPI_COMM_NULL;
  MPI_Comm* comm_world_ = nullptr;
  SmpiProcessState state_;
  int sampling_ = 0; /* inside an SMPI_SAMPLE_ block? */
  std::string instance_id_;
  bool replaying_ = false; /* is the process replaying a trace */
  simgrid::s4u::Barrier* finalization_barrier_;
  smpi_trace_call_location_t trace_call_loc_;
  simgrid::s4u::ActorPtr actor_                  = nullptr;
  smpi_privatization_region_t privatized_region_ = nullptr;
  int optind                                     = 0; /*for getopt replacement */
#if HAVE_PAPI
  /** Contains hardware data as read by PAPI **/
  int papi_event_set_;
  papi_counter_t papi_counter_data_;
#endif
public:
  explicit ActorExt(simgrid::s4u::ActorPtr actor, simgrid::s4u::Barrier* barrier);
  ~ActorExt();
  void set_data(int* argc, char*** argv);
  void finalize();
  int finalized();
  int initializing();
  int initialized();
  void mark_as_initialized();
  void set_replaying(bool value);
  bool replaying();
  smpi_trace_call_location_t* call_location();
  void set_privatized_region(smpi_privatization_region_t region);
  smpi_privatization_region_t privatized_region();
  smx_mailbox_t mailbox();
  smx_mailbox_t mailbox_small();
  xbt_mutex_t mailboxes_mutex();
#if HAVE_PAPI
  int papi_event_set();
  papi_counter_t& papi_counters();
#endif
  xbt_os_timer_t timer();
  void simulated_start();
  double simulated_elapsed();
  MPI_Comm comm_world();
  MPI_Comm comm_self();
  MPI_Comm comm_intra();
  void set_comm_intra(MPI_Comm comm);
  void set_sampling(int s);
  int sampling();
  static void init(int* argc, char*** argv);
  simgrid::s4u::ActorPtr get_actor();
  int get_optind();
  void set_optind(int optind);
};

} // namespace smpi
} // namespace simgrid

#endif
