/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_process.hpp"
#include "mc/mc.h"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_group.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_process, smpi, "Logging specific to SMPI (kernel)");

extern int* index_to_process_data;

#define MAILBOX_NAME_MAXLEN (5 + sizeof(int) * 2 + 1)

static char *get_mailbox_name(char *str, int index)
{
  snprintf(str, MAILBOX_NAME_MAXLEN, "SMPI-%0*x", static_cast<int>(sizeof(int) * 2), static_cast<unsigned>(index));
  return str;
}

static char *get_mailbox_name_small(char *str, int index)
{
  snprintf(str, MAILBOX_NAME_MAXLEN, "small%0*x", static_cast<int>(sizeof(int) * 2), static_cast<unsigned>(index));
  return str;
}

namespace simgrid{
namespace smpi{

Process::Process(int index, msg_bar_t finalization_barrier)
  : finalization_barrier_(finalization_barrier)
{
  char name[MAILBOX_NAME_MAXLEN];
  mailbox_              = simgrid::s4u::Mailbox::byName(get_mailbox_name(name, index));
  mailbox_small_        = simgrid::s4u::Mailbox::byName(get_mailbox_name_small(name, index));
  mailboxes_mutex_      = xbt_mutex_init();
  timer_                = xbt_os_timer_new();
  state_                = SMPI_UNINITIALIZED;
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

void Process::set_data(int index, int* argc, char*** argv)
{
    char* instance_id = (*argv)[1];
    comm_world_       = smpi_deployment_comm_world(instance_id);
    msg_bar_t bar     = smpi_deployment_finalization_barrier(instance_id);
    if (bar != nullptr) // don't overwrite the current one if the instance has none
      finalization_barrier_ = bar;
    instance_id_ = instance_id;
    index_       = index;

    static_cast<simgrid::msg::ActorExt*>(SIMIX_process_self()->userdata)->data = this;

    if (*argc > 3) {
      memmove(&(*argv)[0], &(*argv)[2], sizeof(char *) * (*argc - 2));
      (*argv)[(*argc) - 1] = nullptr;
      (*argv)[(*argc) - 2] = nullptr;
    }
    (*argc)-=2;
    argc_ = argc;
    argv_ = argv;
    // set the process attached to the mailbox
    mailbox_small_->setReceiver(simgrid::s4u::Actor::self());
    process_ = SIMIX_process_self();
    XBT_DEBUG("<%d> New process in the game: %p", index_, SIMIX_process_self());
}

/** @brief Prepares the current process for termination. */
void Process::finalize()
{
  state_ = SMPI_FINALIZED;
  XBT_DEBUG("<%d> Process left the game", index_);

    // This leads to an explosion of the search graph which cannot be reduced:
    if(MC_is_active() || MC_record_replay_is_active())
      return;
    // wait for all pending asynchronous comms to finish
    MSG_barrier_wait(finalization_barrier_);
}

/** @brief Check if a process is finalized */
int Process::finalized()
{
    if (index_ != MPI_UNDEFINED)
      return (state_ == SMPI_FINALIZED);
    else
      return 0;
}

/** @brief Check if a process is initialized */
int Process::initialized()
{
  if (index_to_process_data == nullptr){
    return false;
  } else{
    return ((index_ != MPI_UNDEFINED) && (state_ == SMPI_INITIALIZED));
  }
}

/** @brief Mark a process as initialized (=MPI_Init called) */
void Process::mark_as_initialized()
{
  if ((index_ != MPI_UNDEFINED) && (state_ != SMPI_FINALIZED))
    state_ = SMPI_INITIALIZED;
}

void Process::set_replaying(bool value){
  if ((index_ != MPI_UNDEFINED) && (state_ != SMPI_FINALIZED))
    replaying_ = value;
}

bool Process::replaying(){
  if (index_ != MPI_UNDEFINED)
    return replaying_;
  else
    return false;
}

void Process::set_user_data(void *data)
{
  data_ = data;
}

void *Process::get_user_data()
{
  return data_;
}

smx_actor_t Process::process(){
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

int Process::index()
{
  return index_;
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

papi_counter_t& smpi_process_papi_counters()
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
    group->set_mapping(index_, 0);
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

int Process::return_value(){
  return return_value_;
}

void Process::set_return_value(int val){
  return_value_=val;
}

void Process::init(int *argc, char ***argv){

  if (smpi_process_count() == 0) {
    printf("SimGrid was not initialized properly before entering MPI_Init. Aborting, please check compilation process and use smpirun\n");
    exit(1);
  }
  if (argc != nullptr && argv != nullptr) {
    smx_actor_t proc = SIMIX_process_self();
    proc->context->set_cleanup(&MSG_process_cleanup_from_SIMIX);

    int index = proc->pid - 1; // The maestro process has always ID 0 but we don't need that process here

    if(index_to_process_data == nullptr){
      index_to_process_data=static_cast<int*>(xbt_malloc(SIMIX_process_count()*sizeof(int)));
    }

    char* instance_id = (*argv)[1];
    try {
      int rank = std::stoi(std::string((*argv)[2]));
      smpi_deployment_register_process(instance_id, rank, index);
    } catch (std::invalid_argument& ia) {
      throw std::invalid_argument(std::string("Invalid rank: ") + (*argv)[2]);
    }

    // cheinrich: I'm not sure what the impact of the SMPI_switch_data_segment on this call is. I moved
    // this up here so that I can set the privatized region before the switch.
    Process* process = smpi_process_remote(index);
    if(smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP){
      /* Now using segment index of the process  */
      index = proc->segment_index;
      process->set_privatized_region(smpi_init_global_memory_segment_process());
      /* Done at the process's creation */
      SMPI_switch_data_segment(index);
    }

    process->set_data(index, argc, argv);
  }
  xbt_assert(smpi_process(),
      "smpi_process() returned nullptr. You probably gave a nullptr parameter to MPI_Init. "
      "Although it's required by MPI-2, this is currently not supported by SMPI.");
}

}
}
