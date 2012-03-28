/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "msg_mailbox.h"
#include "mc/mc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/virtu.h"
#include "xbt/ex.h"             /* ex_backtrace_display */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_kernel, msg,
                                "Logging specific to MSG (kernel)");

MSG_Global_t msg_global = NULL;

/********************************* MSG **************************************/

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init_args(int *argc, char **argv)
{
  MSG_global_init(argc, argv);
}

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init(int *argc, char **argv)
{
#ifdef HAVE_TRACING
  TRACE_global_init(argc, argv);
#endif

  xbt_getpid = MSG_process_self_PID;
  if (!msg_global) {
    /* Connect our log channels: that must be done manually under windows */
    XBT_LOG_CONNECT(msg_gos, msg);
    XBT_LOG_CONNECT(msg_kernel, msg);
    XBT_LOG_CONNECT(msg_mailbox, msg);
    XBT_LOG_CONNECT(msg_process, msg);

    SIMIX_global_init(argc, argv);

    msg_global = xbt_new0(s_MSG_Global_t, 1);

#ifdef MSG_USE_DEPRECATED
    msg_global->max_channel = 0;
#endif
    msg_global->PID = 1;
    msg_global->sent_msg = 0;
    msg_global->task_copy_callback = NULL;
    msg_global->process_data_cleanup = NULL;

    /* initialization of the action module */
    _MSG_action_init();

    SIMIX_function_register_process_create(MSG_process_create_from_SIMIX);
    SIMIX_function_register_process_cleanup(MSG_process_cleanup_from_SIMIX);
    SIMIX_function_register_process_kill(MSG_process_kill_from_SIMIX);
  }
#ifdef HAVE_TRACING
  TRACE_start();
#endif

  XBT_DEBUG("ADD MSG LEVELS");
  MSG_HOST_LEVEL = xbt_lib_add_level(host_lib, (void_f_pvoid_t) __MSG_host_destroy);
}

#ifdef MSG_USE_DEPRECATED

/* This deprecated function has to be called to fix the number of channel in the
   simulation before creating any host. Indeed, each channel is
   represented by a different mailbox on each #m_host_t. This
   function can then be called only once. This function takes only one
   parameter.
 * \param number the number of channel in the simulation. It has to be >0
 */
MSG_error_t MSG_set_channel_number(int number)
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
MSG_error_t MSG_main(void)
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_IS_ENABLED) {
    MC_modelcheck();
  }
  else {
    SIMIX_run();
  }
  return MSG_OK;
}

MSG_error_t MSG_main_stateful(void)
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_IS_ENABLED) {
    MC_modelcheck_stateful();
  }
  else {
    SIMIX_run();
  }
  return MSG_OK;
}


MSG_error_t MSG_main_liveness(xbt_automaton_t a, char *prgm)
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_IS_ENABLED) {
    MC_modelcheck_liveness(a, prgm);
  }
  else {
    SIMIX_run();
  }
  return MSG_OK;
}

/** \ingroup msg_simulation
 * \brief Kill all running process

 * \param reset_PIDs should we reset the PID numbers. A negative
 *   number means no reset and a positive number will be used to set the PID
 *   of the next newly created process.
 */
int MSG_process_killall(int reset_PIDs)
{
  simcall_process_killall();

  if (reset_PIDs > 0) {
    msg_global->PID = reset_PIDs;
    msg_global->session++;
  }

  return msg_global->PID;

}

/** \ingroup msg_simulation
 * \brief Clean the MSG simulation
 */
MSG_error_t MSG_clean(void)
{
  XBT_DEBUG("Closing MSG");

#ifdef HAVE_TRACING
  TRACE_surf_release();
#endif

  MSG_process_killall(0);

  /* initialization of the action module */
  _MSG_action_exit();

#ifdef HAVE_TRACING
  TRACE_end();
#endif

  SIMIX_clean();

  free(msg_global);
  msg_global = NULL;

  return MSG_OK;
}


/** \ingroup msg_easier_life
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
