/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/smpi/include/smpi_actor.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Mutex.hpp"
#include "smpi_comm.hpp"
#include "smpi_info.hpp"
#include "src/mc/mc.h"
#include "src/mc/mc_replay.hpp"
#include "xbt/str.h"

#if HAVE_PAPI
#include <papi.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_process, smpi, "Logging specific to SMPI (kernel)");

namespace simgrid::smpi {
simgrid::xbt::Extension<simgrid::s4u::Actor, ActorExt> ActorExt::EXTENSION_ID;

ActorExt::ActorExt(s4u::Actor* actor) : actor_(actor)
{
  if (not simgrid::smpi::ActorExt::EXTENSION_ID.valid())
    simgrid::smpi::ActorExt::EXTENSION_ID = simgrid::s4u::Actor::extension_create<simgrid::smpi::ActorExt>();

  mailbox_         = nullptr;
  mailbox_small_   = nullptr;
  mailboxes_mutex_ = s4u::Mutex::create();
  timer_           = xbt_os_timer_new();
  state_           = SmpiProcessState::UNINITIALIZED;
  info_env_        = MPI_INFO_NULL;

#if HAVE_PAPI
  if (not smpi_cfg_papi_events_file().empty()) {
    // TODO: Implement host/process/thread based counters. This implementation
    // just always takes the values passed via "default", like this:
    // "default:COUNTER1:COUNTER2:COUNTER3;".
    auto it = units2papi_setup.find("default");
    if (it != units2papi_setup.end()) {
      papi_event_set_    = it->second.event_set;
      papi_counter_data_ = it->second.counter_data;
      XBT_DEBUG("Setting PAPI set for process %li", actor->get_pid());
    } else {
      papi_event_set_ = PAPI_NULL;
      XBT_DEBUG("No PAPI set for process %li", actor->get_pid());
    }
  }
#endif
}

ActorExt::~ActorExt()
{
  xbt_os_timer_free(timer_);
}

/** @brief Prepares the current process for termination. */
void ActorExt::finalize()
{
  state_ = SmpiProcessState::FINALIZED;
  XBT_DEBUG("<%ld> Process left the game", actor_->get_pid());
  if (info_env_ != MPI_INFO_NULL)
    simgrid::smpi::Info::unref(info_env_);
  if (comm_self_ != MPI_COMM_NULL)
    simgrid::smpi::Comm::destroy(comm_self_);
  if (comm_intra_ != MPI_COMM_NULL)
    simgrid::smpi::Comm::destroy(comm_intra_);
  smpi_deployment_unregister_process(instance_id_);
}

/** @brief Check if a process is finalized */
int ActorExt::finalized() const
{
  return (state_ == SmpiProcessState::FINALIZED);
}

/** @brief Check if a process is partially initialized already */
int ActorExt::initializing() const
{
  return (state_ == SmpiProcessState::INITIALIZING);
}

/** @brief Check if a process is initialized */
int ActorExt::initialized() const
{
  // TODO cheinrich: Check if we still need this. This should be a global condition, not for a
  // single process ... ?
  return (state_ == SmpiProcessState::INITIALIZED);
}

/** @brief Return main mailbox of the process */
s4u::Mailbox* ActorExt::mailbox()
{
  if(mailbox_==nullptr)
    mailbox_=s4u::Mailbox::by_name("SMPI-" + std::to_string(actor_->get_pid()));
  return mailbox_;
}

/** @brief Return mailbox for small messages */
s4u::Mailbox* ActorExt::mailbox_small()
{
  if(mailbox_small_==nullptr)
    mailbox_small_=s4u::Mailbox::by_name("small-" + std::to_string(actor_->get_pid()));
  return mailbox_small_;
}

/** @brief Mark a process as initialized (=MPI_Init called) */
void ActorExt::mark_as_initialized()
{
  if (state_ != SmpiProcessState::FINALIZED)
    state_ = SmpiProcessState::INITIALIZED;
}

/** @brief Mark a process as finalizing (=MPI_Finalize called) */
void ActorExt::mark_as_finalizing()
{
  if (state_ != SmpiProcessState::FINALIZED)
    state_ = SmpiProcessState::FINALIZING;
}

/** @brief Check if a process is finalizing */
int ActorExt::finalizing() const
{
  return (state_ == SmpiProcessState::FINALIZING);
}

void ActorExt::set_replaying(bool value)
{
  if (state_ != SmpiProcessState::FINALIZED)
    replaying_ = value;
}

bool ActorExt::replaying() const
{
  return replaying_;
}

s4u::ActorPtr ActorExt::get_actor()
{
  return actor_;
}

/**
 * @brief Returns a structure that stores the location (filename + linenumber) of the last calls to MPI_* functions.
 *
 * @see smpi_trace_set_call_location
 */
smpi_trace_call_location_t* ActorExt::call_location()
{
  return &trace_call_loc_;
}

void ActorExt::set_privatized_region(smpi_privatization_region_t region)
{
  privatized_region_ = region;
}

smpi_privatization_region_t ActorExt::privatized_region() const
{
  return privatized_region_;
}

MPI_Comm ActorExt::comm_world() const
{
  return comm_world_ == nullptr ? MPI_COMM_NULL : *comm_world_;
}

s4u::MutexPtr ActorExt::mailboxes_mutex() const
{
  return mailboxes_mutex_;
}

#if HAVE_PAPI
int ActorExt::papi_event_set() const
{
  return papi_event_set_;
}

papi_counter_t& ActorExt::papi_counters()
{
  return papi_counter_data_;
}
#endif

xbt_os_timer_t ActorExt::timer()
{
  return timer_;
}

void ActorExt::simulated_start()
{
  simulated_ = s4u::Engine::get_clock();
}

double ActorExt::simulated_elapsed() const
{
  return s4u::Engine::get_clock() - simulated_;
}

MPI_Comm ActorExt::comm_self()
{
  if (comm_self_ == MPI_COMM_NULL) {
    auto* group = new Group(1);
    comm_self_  = new Comm(group, nullptr);
    comm_self_->set_name("MPI_COMM_SELF");
    group->set_mapping(actor_->get_pid(), 0);
  }
  return comm_self_;
}

MPI_Info ActorExt::info_env()
{
  if (info_env_==MPI_INFO_NULL)
    info_env_=new Info();
  return info_env_;
}

MPI_Comm ActorExt::comm_intra()
{
  return comm_intra_;
}

void ActorExt::set_comm_intra(MPI_Comm comm)
{
  comm_intra_ = comm;
}

void ActorExt::set_sampling(int s)
{
  sampling_ = s;
}

int ActorExt::sampling() const
{
  return sampling_;
}

void ActorExt::init()
{
  ActorExt* ext = smpi_process();
  // if we are in MPI_Init and argc handling has already been done.
  if (ext->initialized())
    return;

  const simgrid::s4u::Actor* self = simgrid::s4u::Actor::self();
  const char* id                  = self->get_property("instance_id");
  xbt_assert(id != nullptr, "Actor '%s' seem to be calling MPI_Init(), but it was created outside of MPI, wasn't it?",
             self->get_cname());
  ext->instance_id_ = id;
  const int rank = static_cast<int>(xbt_str_parse_int(self->get_property("rank"), "Cannot parse rank"));

  ext->state_ = SmpiProcessState::INITIALIZING;
  smpi_deployment_register_process(ext->instance_id_, rank, self);

  ext->comm_world_ = smpi_deployment_comm_world(ext->instance_id_);

  // set the process attached to the mailbox
  ext->mailbox_small()->set_receiver(ext->actor_);
  XBT_DEBUG("<%ld> SMPI process has been initialized: %p", ext->actor_->get_pid(), ext->actor_);
}

int ActorExt::get_optind() const
{
  return optind_;
}

void ActorExt::set_optind(int new_optind)
{
  optind_ = new_optind;
}

void ActorExt::bsend_buffer(void** buf, int* size)
{
  *buf  = bsend_buffer_;
  *size = bsend_buffer_size_;
}

int ActorExt::set_bsend_buffer(void* buf, int size)
{
  if(buf!=nullptr && bsend_buffer_!=nullptr)
    return MPI_ERR_BUFFER;
  bsend_buffer_     = buf;
  bsend_buffer_size_= size;
  return MPI_SUCCESS;
}

} // namespace simgrid::smpi
