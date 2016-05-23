/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMIX_PROCESS_PRIVATE_H
#define _SIMIX_PROCESS_PRIVATE_H

#include <functional>
#include <string>

#include <xbt/base.h>
#include <xbt/string.hpp>

#include <simgrid/simix.hpp>
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
  void *data = nullptr;
  const char *hostname = nullptr;
  double kill_time = 0.0;
  xbt_dict_t properties  = nullptr;
  bool auto_restart = false;
};

class Process {
public:

  // TODO, replace with boost intrusive container hooks
  s_xbt_swag_hookup_t process_hookup = { nullptr, nullptr };   /* simix_global->process_list */
  s_xbt_swag_hookup_t synchro_hookup = { nullptr, nullptr };   /* {mutex,cond,sem}->sleeping */
  s_xbt_swag_hookup_t host_proc_hookup = { nullptr, nullptr }; /* smx_host->process_lis */
  s_xbt_swag_hookup_t destroy_hookup = { nullptr, nullptr };   /* simix_global->process_to_destroy */

  unsigned long pid = 0;
  unsigned long ppid = 0;
  simgrid::xbt::string name;
  sg_host_t host = nullptr;     /* the host on which the process is running */
  smx_context_t context = nullptr; /* the context (uctx/raw/thread) that executes the user function */
  xbt_running_ctx_t *running_ctx = nullptr;

  // TODO, pack them
  bool doexception = false;
  bool blocked = false;
  bool suspended = false;
  bool auto_restart = false;

  sg_host_t new_host = nullptr;     /* if not null, the host on which the process must migrate to */
  smx_synchro_t waiting_synchro = nullptr;  /* the current blocking synchro if any */
  xbt_fifo_t comms = nullptr;       /* the current non-blocking communication synchros */
  xbt_dict_t properties = nullptr;
  s_smx_simcall_t simcall;
  void *data = nullptr;    /* kept for compatibility, it should be replaced with moddata */
  xbt_dynar_t on_exit = nullptr; /* list of functions executed when the process dies */

  std::function<void()> code;
  smx_timer_t kill_timer = nullptr;
  int segment_index = 0;    /*Reference to an SMPI process' data segment. Default value is -1 if not in SMPI context*/
};

}
}

typedef simgrid::simix::ProcessArg *smx_process_arg_t;

typedef simgrid::simix::Process* smx_process_t;

SG_BEGIN_DECL()

XBT_PRIVATE smx_process_t SIMIX_process_create(
                          const char *name,
                          std::function<void()> code,
                          void *data,
                          const char *hostname,
                          double kill_time,
                          xbt_dict_t properties,
                          int auto_restart,
                          smx_process_t parent_process);

XBT_PRIVATE void SIMIX_process_runall(void);
XBT_PRIVATE void SIMIX_process_kill(smx_process_t process, smx_process_t issuer);
XBT_PRIVATE void SIMIX_process_killall(smx_process_t issuer, int reset_pid);
XBT_PRIVATE void SIMIX_process_stop(smx_process_t arg);
XBT_PRIVATE void SIMIX_process_cleanup(smx_process_t arg);
XBT_PRIVATE void SIMIX_process_empty_trash(void);
XBT_PRIVATE void SIMIX_process_yield(smx_process_t self);
XBT_PRIVATE xbt_running_ctx_t *SIMIX_process_get_running_context(void);
XBT_PRIVATE void SIMIX_process_exception_terminate(xbt_ex_t * e);
XBT_PRIVATE void SIMIX_process_change_host(smx_process_t process,
             sg_host_t dest);
XBT_PRIVATE smx_synchro_t SIMIX_process_suspend(smx_process_t process, smx_process_t issuer);
XBT_PRIVATE void SIMIX_process_resume(smx_process_t process, smx_process_t issuer);
XBT_PRIVATE int SIMIX_process_get_PID(smx_process_t self);
XBT_PRIVATE int SIMIX_process_get_PPID(smx_process_t self);
XBT_PRIVATE void* SIMIX_process_get_data(smx_process_t process);
XBT_PRIVATE void SIMIX_process_set_data(smx_process_t process, void *data);
XBT_PRIVATE sg_host_t SIMIX_process_get_host(smx_process_t process);
XBT_PRIVATE const char* SIMIX_process_get_name(smx_process_t process);
XBT_PRIVATE smx_process_t SIMIX_process_get_by_name(const char* name);
XBT_PRIVATE int SIMIX_process_is_suspended(smx_process_t process);
XBT_PRIVATE xbt_dict_t SIMIX_process_get_properties(smx_process_t process);
XBT_PRIVATE smx_synchro_t SIMIX_process_join(smx_process_t issuer, smx_process_t process, double timeout);
XBT_PRIVATE smx_synchro_t SIMIX_process_sleep(smx_process_t process, double duration);

XBT_PRIVATE void SIMIX_process_sleep_destroy(smx_synchro_t synchro);
XBT_PRIVATE void SIMIX_process_auto_restart_set(smx_process_t process, int auto_restart);
XBT_PRIVATE smx_process_t SIMIX_process_restart(smx_process_t process, smx_process_t issuer);

void SIMIX_segment_index_set(smx_process_t, int);
extern void (*SMPI_switch_data_segment)(int);

SG_END_DECL()

#endif
