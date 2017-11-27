/* Copyright (c) 2009-2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PROCESS_HPP
#define SMPI_PROCESS_HPP

#include "private.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/instr/instr_smpi.hpp"
#include "xbt/synchro.h"

namespace simgrid{
namespace smpi{

class Process {
  private:
    double simulated_ = 0 /* Used to time with simulated_start/elapsed */;
    int* argc_        = nullptr;
    char*** argv_     = nullptr;
    simgrid::s4u::MailboxPtr mailbox_;
    simgrid::s4u::MailboxPtr mailbox_small_;
    xbt_mutex_t mailboxes_mutex_;
    xbt_os_timer_t timer_;
    MPI_Comm comm_self_   = MPI_COMM_NULL;
    MPI_Comm comm_intra_  = MPI_COMM_NULL;
    MPI_Comm* comm_world_ = nullptr;
    void* data_           = nullptr; /* user data */
    int index_            = MPI_UNDEFINED;
    char state_;
    int sampling_                   = 0; /* inside an SMPI_SAMPLE_ block? */
    char* instance_id_              = nullptr;
    bool replaying_                 = false; /* is the process replaying a trace */
    msg_bar_t finalization_barrier_;
    int return_value_ = 0;
    smpi_trace_call_location_t trace_call_loc_;
    smx_actor_t process_ = nullptr;
    smpi_privatization_region_t privatized_region_;
#if HAVE_PAPI
  /** Contains hardware data as read by PAPI **/
    int papi_event_set_;
    papi_counter_t papi_counter_data_;
#endif
  public:
    explicit Process(int index, msg_bar_t barrier);
    void set_data(int index, int* argc, char*** argv);
    void finalize();
    int finalized();
    int initialized();
    void mark_as_initialized();
    void set_replaying(bool value);
    bool replaying();
    void set_user_data(void *data);
    void *get_user_data();
    smpi_trace_call_location_t* call_location();
    void set_privatized_region(smpi_privatization_region_t region);
    smpi_privatization_region_t privatized_region();
    int index();
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
    msg_bar_t finalization_barrier();
    int return_value();
    void set_return_value(int val);
    static void init(int *argc, char ***argv);
    smx_actor_t process();
};


}
}

#endif
