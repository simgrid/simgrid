/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"

#include "instr/instr_interface.h"
#include "mc/mc.h"
#include "src/msg/msg_private.hpp"

XBT_LOG_NEW_CATEGORY(msg, "All MSG categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_kernel, msg, "Logging specific to MSG (kernel)");

extern "C" {

MSG_Global_t msg_global = nullptr;
static void MSG_exit();

/********************************* MSG **************************************/

static void _sg_cfg_cb_msg_debug_multiple_use(const char *name)
{
  msg_global->debug_multiple_use = xbt_cfg_get_boolean(name);
}

/**
 * \ingroup msg_simulation
 * \brief Initialize MSG with less verifications
 * You should use the MSG_init() function instead. Failing to do so may turn into PEBKAC some day. You've been warned.
 */
void MSG_init_nocheck(int *argc, char **argv) {

  TRACE_global_init();

  if (not msg_global) {

    msg_global = new s_MSG_Global_t();

    xbt_cfg_register_boolean("msg/debug-multiple-use", "no", _sg_cfg_cb_msg_debug_multiple_use,
        "Print backtraces of both processes when there is a conflict of multiple use of a task");

    SIMIX_global_init(argc, argv);

    msg_global->sent_msg = 0;
    msg_global->task_copy_callback = nullptr;
    msg_global->process_data_cleanup = nullptr;

    SIMIX_function_register_process_create(MSG_process_create_from_SIMIX);
    SIMIX_function_register_process_cleanup(MSG_process_cleanup_from_SIMIX);
  }

  if(MC_is_active()){
    /* Ignore total amount of messages sent during the simulation for heap comparison */
    MC_ignore_heap(&(msg_global->sent_msg), sizeof(msg_global->sent_msg));
  }

  if (xbt_cfg_get_boolean("clean-atexit"))
    atexit(MSG_exit);
}

/** \ingroup msg_simulation
 * \brief Launch the MSG simulation
 */
msg_error_t MSG_main()
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();
  }
  return MSG_OK;
}

/** \ingroup msg_simulation
 * \brief set a configuration variable
 *
 * Do --help on any simgrid binary to see the list of currently existing configuration variables, and see Section @ref options.
 *
 * Example:
 * MSG_config("host/model","ptask_L07");
 */
void MSG_config(const char *key, const char *value){
  xbt_assert(msg_global,"ERROR: Please call MSG_init() before using MSG_config()");
  xbt_cfg_set_as_string(key, value);
}

/** \ingroup msg_simulation
 * \brief Kill all running process

 * \param reset_PIDs should we reset the PID numbers. A negative
 *   number means no reset and a positive number will be used to set the PID
 *   of the next newly created process.
 */
int MSG_process_killall(int reset_PIDs)
{
  simcall_process_killall(reset_PIDs);

  return 0;
}

static void MSG_exit() {
  if (msg_global==nullptr)
    return;

  TRACE_end();
  delete msg_global;
  msg_global = nullptr;
}

/** \ingroup msg_simulation
 * \brief A clock (in second).
 */
double MSG_get_clock()
{
  return SIMIX_get_clock();
}

unsigned long int MSG_get_sent_msg()
{
  return msg_global->sent_msg;
}
}
