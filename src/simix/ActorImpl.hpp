/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_ACTORIMPL_H
#define _SIMIX_ACTORIMPL_H

#include <atomic>
#include <functional>
#include <string>

#include <xbt/base.h>
#include <xbt/string.hpp>

#include <simgrid/simix.hpp>
#include <simgrid/s4u/Actor.hpp>

#include "simgrid/simix.h"
#include "popping_private.h"

typedef struct s_smx_process_exit_fun {
  int_f_pvoid_pvoid_t fun;
  void *arg;
} s_smx_process_exit_fun_t, *smx_process_exit_fun_t;

namespace simgrid {
namespace simix {

class ProcessArg {
public:
  std::string name;
  std::function<void()> code;
  void *data            = nullptr;
  const char *hostname  = nullptr;
  double kill_time      = 0.0;
  xbt_dict_t properties = nullptr;
  bool auto_restart     = false;
};

class ActorImpl {
public:
  ActorImpl() : piface_(this) {}

  // TODO, replace with boost intrusive container hooks
  s_xbt_swag_hookup_t process_hookup   = { nullptr, nullptr }; /* simix_global->process_list */
  s_xbt_swag_hookup_t synchro_hookup   = { nullptr, nullptr }; /* {mutex,cond,sem}->sleeping */
  s_xbt_swag_hookup_t host_proc_hookup = { nullptr, nullptr }; /* smx_host->process_lis */
  s_xbt_swag_hookup_t destroy_hookup   = { nullptr, nullptr }; /* simix_global->process_to_destroy */

  unsigned long pid  = 0;
  unsigned long ppid = 0;
  simgrid::xbt::string name;
  sg_host_t host        = nullptr; /* the host on which the process is running */
  smx_context_t context = nullptr; /* the context (uctx/raw/thread) that executes the user function */

  // TODO, pack them
  std::exception_ptr exception;
  bool finished     = false;
  bool blocked      = false;
  bool suspended    = false;
  bool auto_restart = false;

  sg_host_t new_host            = nullptr; /* if not null, the host on which the process must migrate to */
  smx_activity_t waiting_synchro = nullptr; /* the current blocking synchro if any */
  xbt_fifo_t comms              = nullptr; /* the current non-blocking communication synchros */
  xbt_dict_t properties         = nullptr;
  s_smx_simcall_t simcall;
  void *data          = nullptr; /* kept for compatibility, it should be replaced with moddata */
  xbt_dynar_t on_exit = nullptr; /* list of functions executed when the process dies */

  std::function<void()> code;
  smx_timer_t kill_timer = nullptr;
  int segment_index      = 0; /* Reference to an SMPI process' data segment. Default value is -1 if not in SMPI context*/

  friend void intrusive_ptr_add_ref(ActorImpl* process)
  {
    // Atomic operation! Do not split in two instructions!
    auto previous = (process->refcount_)++;
    xbt_assert(previous != 0);
    (void) previous;
  }
  friend void intrusive_ptr_release(ActorImpl* process)
  {
    // Atomic operation! Do not split in two instructions!
    auto count = --(process->refcount_);
    if (count == 0)
      delete process;
  }

  ~ActorImpl();

  simgrid::s4u::Actor& getIface() { return piface_; }

private:
  std::atomic_int_fast32_t refcount_ { 1 };
  simgrid::s4u::Actor piface_;
};

}
}

typedef simgrid::simix::ProcessArg *smx_process_arg_t;

typedef simgrid::simix::ActorImpl* smx_actor_t;

SG_BEGIN_DECL()

XBT_PRIVATE smx_actor_t SIMIX_process_create(
                          const char *name,
                          std::function<void()> code,
                          void *data,
                          const char *hostname,
                          double kill_time,
                          xbt_dict_t properties,
                          int auto_restart,
                          smx_actor_t parent_process);

XBT_PRIVATE void SIMIX_process_runall();
XBT_PRIVATE void SIMIX_process_kill(smx_actor_t process, smx_actor_t issuer);
XBT_PRIVATE void SIMIX_process_killall(smx_actor_t issuer, int reset_pid);
XBT_PRIVATE void SIMIX_process_stop(smx_actor_t arg);
XBT_PRIVATE void SIMIX_process_cleanup(smx_actor_t arg);
XBT_PRIVATE void SIMIX_process_empty_trash();
XBT_PRIVATE void SIMIX_process_yield(smx_actor_t self);
XBT_PRIVATE void SIMIX_process_exception_terminate(xbt_ex_t * e);
XBT_PRIVATE void SIMIX_process_change_host(smx_actor_t process, sg_host_t dest);
XBT_PRIVATE smx_activity_t SIMIX_process_suspend(smx_actor_t process, smx_actor_t issuer);
XBT_PRIVATE void SIMIX_process_resume(smx_actor_t process, smx_actor_t issuer);
XBT_PRIVATE int SIMIX_process_get_PID(smx_actor_t self);
XBT_PRIVATE void* SIMIX_process_get_data(smx_actor_t process);
XBT_PRIVATE void SIMIX_process_set_data(smx_actor_t process, void *data);
XBT_PRIVATE smx_actor_t SIMIX_process_get_by_name(const char* name);
XBT_PRIVATE int SIMIX_process_is_suspended(smx_actor_t process);
XBT_PRIVATE xbt_dict_t SIMIX_process_get_properties(smx_actor_t process);
XBT_PRIVATE smx_activity_t SIMIX_process_join(smx_actor_t issuer, smx_actor_t process, double timeout);
XBT_PRIVATE smx_activity_t SIMIX_process_sleep(smx_actor_t process, double duration);

XBT_PRIVATE void SIMIX_process_sleep_destroy(smx_activity_t synchro);
XBT_PRIVATE void SIMIX_process_auto_restart_set(smx_actor_t process, int auto_restart);
XBT_PRIVATE smx_actor_t SIMIX_process_restart(smx_actor_t process, smx_actor_t issuer);

void SIMIX_segment_index_set(smx_actor_t process, int segment_index);
extern void (*SMPI_switch_data_segment)(int dest);

SG_END_DECL()

#endif
