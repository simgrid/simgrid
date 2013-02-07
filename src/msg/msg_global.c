/* Copyright (c) 2004-2012. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_interface.h"
#include "msg_private.h"
#include "msg_mailbox.h"
#include "mc/mc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/virtu.h"
#include "xbt/ex.h"             /* ex_backtrace_display */
#include "xbt/replay.h"
#include "simgrid/sg_config.h" /* Configuration mechanism of SimGrid */


XBT_LOG_NEW_CATEGORY(msg, "All MSG categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_kernel, msg,
                                "Logging specific to MSG (kernel)");

MSG_Global_t msg_global = NULL;
static void MSG_exit(void);

/********************************* MSG **************************************/

/**
 * \ingroup msg_simulation
 * \brief Initialize MSG with less verifications
 * You should use the MSG_init() function instead. Failing to do so may turn into PEBKAC some day. You've been warned.
 */
void MSG_init_nocheck(int *argc, char **argv) {

#ifdef HAVE_TRACING
  TRACE_global_init(argc, argv);
#endif

  xbt_getpid = MSG_process_self_PID;
  if (!msg_global) {

    SIMIX_global_init(argc, argv);
    
    msg_global = xbt_new0(s_MSG_Global_t, 1);

#ifdef MSG_USE_DEPRECATED
    msg_global->max_channel = 0;
#endif
    msg_global->sent_msg = 0;
    msg_global->task_copy_callback = NULL;
    msg_global->process_data_cleanup = NULL;

    /* initialization of the action module */
    _MSG_action_init();

    SIMIX_function_register_process_create(MSG_process_create_from_SIMIX);
    SIMIX_function_register_process_cleanup(MSG_process_cleanup_from_SIMIX);

    sg_platf_postparse_add_cb(MSG_post_create_environment);
  }
  
  if(MC_is_active()){
    /* Ignore total amount of messages sent during the simulation for heap comparison */
    MC_ignore_heap(&(msg_global->sent_msg), sizeof(msg_global->sent_msg));
  }

#ifdef HAVE_TRACING
  TRACE_start();
#endif

  XBT_DEBUG("ADD MSG LEVELS");
  MSG_HOST_LEVEL = xbt_lib_add_level(host_lib, (void_f_pvoid_t) __MSG_host_destroy);

  atexit(MSG_exit);
}

#ifdef MSG_USE_DEPRECATED

/* This deprecated function has to be called to fix the number of channel in the
   simulation before creating any host. Indeed, each channel is
   represented by a different mailbox on each #m_host_t. This
   function can then be called only once. This function takes only one
   parameter.
 * \param number the number of channel in the simulation. It has to be >0
 */
msg_error_t MSG_set_channel_number(int number)
{
  XBT_WARN("DEPRECATED! Please use aliases instead");
  xbt_assert((msg_global)
              && (msg_global->max_channel == 0),
              "Channel number already set!");

  msg_global->max_channel = number;

  return MSG_OK;
}

/* This deprecated function has to be called once the number of channel is fixed. I can't
   figure out a reason why anyone would like to call this function but nevermind.
 * \return the number of channel in the simulation.
 */
int MSG_get_channel_number(void)
{
  XBT_WARN("DEPRECATED! Please use aliases instead");
  xbt_assert((msg_global)
              && (msg_global->max_channel != 0),
              "Channel number not set yet!");

  return msg_global->max_channel;
}
#endif

/** \ingroup msg_simulation
 * \brief Launch the MSG simulation
 */
msg_error_t MSG_main(void)
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_do_the_modelcheck_for_real();
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
 * MSG_config("workstation/model","KCCFLN05");
 */
void MSG_config(const char *key, const char *value){
  xbt_assert(msg_global,"ERROR: Please call MSG_init() before using MSG_config()");
  xbt_cfg_set_as_string(_sg_cfg_set, key, value);
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

  if (reset_PIDs > 0)
    msg_global->session++;

  return 0;

}

static void MSG_exit(void) {
  if (msg_global==NULL)
    return;

#ifdef HAVE_TRACING
  TRACE_surf_resource_utilization_release();
#endif

  /* initialization of the action module */
  _MSG_action_exit();

#ifdef HAVE_TRACING
  TRACE_end();
#endif

  free(msg_global);
  msg_global = NULL;
}


/** \ingroup msg_simulation
 * \brief A clock (in second).
 */
XBT_INLINE double MSG_get_clock(void)
{
  return SIMIX_get_clock();
}

unsigned long int MSG_get_sent_msg()
{
  return msg_global->sent_msg;
}

#ifdef MSG_USE_DEPRECATED
msg_error_t MSG_clean(void) {
  return MSG_OK;
}
#endif
