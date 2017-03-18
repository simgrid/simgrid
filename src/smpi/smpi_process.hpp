/* Copyright (c) 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_PROCESS_HPP
#define SMPI_PROCESS_HPP


#include <xbt/base.h>
#include "src/instr/instr_smpi.h"
#include "private.h"
#include "simgrid/s4u/Mailbox.hpp"

namespace simgrid{
namespace smpi{

class Process {
  private:
    double simulated_;
    int *argc_;
    char ***argv_;
    simgrid::s4u::MailboxPtr mailbox_;
    simgrid::s4u::MailboxPtr mailbox_small_;
    xbt_mutex_t mailboxes_mutex_;
    xbt_os_timer_t timer_;
    MPI_Comm comm_self_;
    MPI_Comm comm_intra_;
    MPI_Comm* comm_world_;
    void *data_;                   /* user data */
    int index_;
    char state_;
    int sampling_;                 /* inside an SMPI_SAMPLE_ block? */
    char* instance_id_;
    bool replaying_;                /* is the process replaying a trace */
    msg_bar_t finalization_barrier_;
    int return_value_;
    smpi_trace_call_location_t trace_call_loc_;
#if HAVE_PAPI
  /** Contains hardware data as read by PAPI **/
    int papi_event_set_;
    papi_counter_t papi_counter_data_;
#endif
  public:
    Process(int index);
    void destroy();
    void set_data(int index, int *argc, char ***argv);
    void finalize();
    int finalized();
    int initialized();
    void mark_as_initialized();
    void set_replaying(bool value);
    bool replaying();
    void set_user_data(void *data);
    void *get_user_data();
    smpi_trace_call_location_t* call_location();
    int index();
    MPI_Comm comm_world();
    smx_mailbox_t mailbox();
    smx_mailbox_t mailbox_small();
    xbt_mutex_t mailboxes_mutex();
    #if HAVE_PAPI
    int papi_event_set(void);
    papi_counter_t& papi_counters(void);
    #endif
    xbt_os_timer_t timer();
    void simulated_start();
    double simulated_elapsed();
    MPI_Comm comm_self();
    MPI_Comm comm_intra();
    void set_comm_intra(MPI_Comm comm);
    void set_sampling(int s);
    int sampling();
    msg_bar_t finalization_barrier();
    void set_finalization_barrier(msg_bar_t bar);
    int return_value();
    void set_return_value(int val);
    static void init(int *argc, char ***argv);
};


}
}

#endif
