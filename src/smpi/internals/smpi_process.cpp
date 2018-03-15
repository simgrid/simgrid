/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_process.hpp"
#include "mc/mc.h"
#include "private.hpp"
#include "simgrid/s4u/forward.hpp"
#include "smpi_comm.hpp"
#include "smpi_group.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_process, smpi, "Logging specific to SMPI (kernel)");

namespace simgrid{
namespace smpi{

using simgrid::s4u::Actor;
using simgrid::s4u::ActorPtr;

Process::Process(ActorPtr actor, msg_bar_t finalization_barrier)
    : finalization_barrier_(finalization_barrier), process_(actor)
{
  mailbox_         = simgrid::s4u::Mailbox::byName("SMPI-" + std::to_string(process_->getPid()));
  mailbox_small_   = simgrid::s4u::Mailbox::byName("small-" + std::to_string(process_->getPid()));
  mailboxes_mutex_ = xbt_mutex_init();
  timer_           = xbt_os_timer_new();
  state_           = SMPI_UNINITIALIZED;
  if (MC_is_active())
    MC_ignore_heap(timer_, xbt_os_timer_size());

#if HAVE_PAPI
  if (xbt_cfg_get_string("smpi/papi-events")[0] != '\0') {
    // TODO: Implement host/process/thread based counters. This implementation
    // just always takes the values passed via "default", like this:
    // "default:COUNTER1:COUNTER2:COUNTER3;".
    auto it = units2papi_setup.find(papi_default_config_name);
    if (it != units2papi_setup.end()) {
      papi_event_set_    = it->second.event_set;
      papi_counter_data_ = it->second.counter_data;
      XBT_DEBUG("Setting PAPI set for process %i", i);
    } else {
      papi_event_set_ = PAPI_NULL;
      XBT_DEBUG("No PAPI set for process %i", i);
    }
  }
#endif
}

Process::~Process()
{
  if (comm_self_ != MPI_COMM_NULL)
    simgrid::smpi::Comm::destroy(comm_self_);
  if (comm_intra_ != MPI_COMM_NULL)
    simgrid::smpi::Comm::destroy(comm_intra_);
  xbt_os_timer_free(timer_);
  xbt_mutex_destroy(mailboxes_mutex_);
}

void Process::set_data(int* argc, char*** argv)
{
  instance_id_      = std::string((*argv)[1]);
  comm_world_       = smpi_deployment_comm_world(instance_id_.c_str());
  msg_bar_t barrier = smpi_deployment_finalization_barrier(instance_id_.c_str());
  if (barrier != nullptr) // don't overwrite the current one if the instance has none
    finalization_barrier_ = barrier;

  process_                                                                  = simgrid::s4u::Actor::self();
  static_cast<simgrid::msg::ActorExt*>(process_->getImpl()->userdata)->data = this;

  if (*argc > 3) {
    memmove(&(*argv)[0], &(*argv)[2], sizeof(char*) * (*argc - 2));
    (*argv)[(*argc) - 1] = nullptr;
    (*argv)[(*argc) - 2] = nullptr;
  }
  (*argc) -= 2;
  argc_ = argc;
  argv_ = argv;
  // set the process attached to the mailbox
  mailbox_small_->setReceiver(process_);
  XBT_DEBUG("<%ld> SMPI process has been initialized: %p", process_->getPid(), process_.get());
}

/** @brief Prepares the current process for termination. */
void Process::finalize()
{
  state_ = SMPI_FINALIZED;
  XBT_DEBUG("<%ld> Process left the game", process_->getPid());

  // This leads to an explosion of the search graph which cannot be reduced:
  if(MC_is_active() || MC_record_replay_is_active())
    return;
  // wait for all pending asynchronous comms to finish
  MSG_barrier_wait(finalization_barrier_);
}

/** @brief Check if a process is finalized */
int Process::finalized()
{
  return (state_ == SMPI_FINALIZED);
}

/** @brief Check if a process is initialized */
int Process::initialized()
{
  // TODO cheinrich: Check if we still need this. This should be a global condition, not for a
  // single process ... ?
  return (state_ == SMPI_INITIALIZED);
}

/** @brief Mark a process as initialized (=MPI_Init called) */
void Process::mark_as_initialized()
{
  if (state_ != SMPI_FINALIZED)
    state_ = SMPI_INITIALIZED;
}

void Process::set_replaying(bool value){
  if (state_ != SMPI_FINALIZED)
    replaying_ = value;
}

bool Process::replaying(){
  return replaying_;
}

void Process::set_user_data(void *data)
{
  data_ = data;
}

void *Process::get_user_data()
{
  return data_;
}

ActorPtr Process::process(){
  return process_;
}

/**
 * \brief Returns a structure that stores the location (filename + linenumber) of the last calls to MPI_* functions.
 *
 * \see smpi_trace_set_call_location
 */
smpi_trace_call_location_t* Process::call_location()
{
  return &trace_call_loc_;
}

void Process::set_privatized_region(smpi_privatization_region_t region)
{
  privatized_region_ = region;
}

smpi_privatization_region_t Process::privatized_region()
{
  return privatized_region_;
}

MPI_Comm Process::comm_world()
{
  return comm_world_==nullptr ? MPI_COMM_NULL : *comm_world_;
}

smx_mailbox_t Process::mailbox()
{
  return mailbox_->getImpl();
}

smx_mailbox_t Process::mailbox_small()
{
  return mailbox_small_->getImpl();
}

xbt_mutex_t Process::mailboxes_mutex()
{
  return mailboxes_mutex_;
}

#if HAVE_PAPI
int Process::papi_event_set()
{
  return papi_event_set_;
}

papi_counter_t& Process::papi_counters()
{
  return papi_counter_data_;
}
#endif

xbt_os_timer_t Process::timer()
{
  return timer_;
}

void Process::simulated_start()
{
  simulated_ = SIMIX_get_clock();
}

double Process::simulated_elapsed()
{
  return SIMIX_get_clock() - simulated_;
}

MPI_Comm Process::comm_self()
{
  if(comm_self_==MPI_COMM_NULL){
    MPI_Group group = new  Group(1);
    comm_self_ = new  Comm(group, nullptr);
    group->set_mapping(process_, 0);
  }
  return comm_self_;
}

MPI_Comm Process::comm_intra()
{
  return comm_intra_;
}

void Process::set_comm_intra(MPI_Comm comm)
{
  comm_intra_ = comm;
}

void Process::set_sampling(int s)
{
  sampling_ = s;
}

int Process::sampling()
{
  return sampling_;
}

msg_bar_t Process::finalization_barrier(){
  return finalization_barrier_;
}

void Process::init(int *argc, char ***argv){

  if (smpi_process_count() == 0) {
    xbt_die("SimGrid was not initialized properly before entering MPI_Init. Aborting, please check compilation process and use smpirun\n");
  }
  if (argc != nullptr && argv != nullptr) {
    simgrid::s4u::ActorPtr proc = simgrid::s4u::Actor::self();
    proc->getImpl()->context->set_cleanup(&MSG_process_cleanup_from_SIMIX);

    char* instance_id = (*argv)[1];
    try {
      int rank = std::stoi(std::string((*argv)[2]));
      smpi_deployment_register_process(instance_id, rank, proc);
    } catch (std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Invalid rank: ") + (*argv)[2]);
    }

    // cheinrich: I'm not sure what the impact of the SMPI_switch_data_segment on this call is. I moved
    // this up here so that I can set the privatized region before the switch.
    Process* process = smpi_process_remote(proc);
    if(smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP){
      /* Now using the segment index of this process  */
      process->set_privatized_region(smpi_init_global_memory_segment_process());
      /* Done at the process's creation */
      SMPI_switch_data_segment(proc);
    }

    process->set_data(argc, argv);
  }
  xbt_assert(smpi_process(), "smpi_process() returned nullptr. You probably gave a nullptr parameter to MPI_Init. "
                             "Although it's required by MPI-2, this is currently not supported by SMPI. "
                             "Please use MPI_Init(&argc, &argv) as usual instead.");
}

}
}
