/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "instr/instr_interface.h"
#include "msg_private.h"
#include "mc/mc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "simgrid/sg_config.h" /* Configuration mechanism of SimGrid */
#include "src/surf/xml/platf_private.hpp" // FIXME: KILLME by removing MSG_post_create_environment()

XBT_LOG_NEW_CATEGORY(msg, "All MSG categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(msg_kernel, msg, "Logging specific to MSG (kernel)");

MSG_Global_t msg_global = NULL;
static void MSG_exit(void);

/********************************* MSG **************************************/

static void _sg_cfg_cb_msg_debug_multiple_use(const char *name, int pos)
{
  msg_global->debug_multiple_use = xbt_cfg_get_boolean(_sg_cfg_set, name);
}

static void MSG_host_create_(sg_host_t host)
{
  __MSG_host_create(host);
}

/**
 * \ingroup msg_simulation
 * \brief Initialize MSG with less verifications
 * You should use the MSG_init() function instead. Failing to do so may turn into PEBKAC some day. You've been warned.
 */
void MSG_init_nocheck(int *argc, char **argv) {

  TRACE_global_init(argc, argv);

  xbt_getpid = MSG_process_self_PID;
  if (!msg_global) {

    msg_global = xbt_new0(s_MSG_Global_t, 1);

    xbt_cfg_register(&_sg_cfg_set, "msg/debug_multiple_use",
                     "Print backtraces of both processes when there is a conflict of multiple use of a task",
                     xbt_cfgelm_boolean, 1, 1, _sg_cfg_cb_msg_debug_multiple_use);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "msg/debug_multiple_use", "no");

    SIMIX_global_init(argc, argv);

    msg_global->sent_msg = 0;
    msg_global->task_copy_callback = NULL;
    msg_global->process_data_cleanup = NULL;

    SIMIX_function_register_process_create(MSG_process_create_from_SIMIX);
    SIMIX_function_register_process_cleanup(MSG_process_cleanup_from_SIMIX);

    simgrid::surf::on_postparse.connect(MSG_post_create_environment);
    simgrid::s4u::Host::onCreation.connect([](simgrid::s4u::Host& host) {
      MSG_host_create_(&host);
    });
    MSG_HOST_LEVEL = simgrid::s4u::Host::extension_create([](void *p) {
      __MSG_host_priv_free((msg_host_priv_t) p);
    });

  }

  if(MC_is_active()){
    /* Ignore total amount of messages sent during the simulation for heap comparison */
    MC_ignore_heap(&(msg_global->sent_msg), sizeof(msg_global->sent_msg));
  }

  XBT_DEBUG("ADD MSG LEVELS");
  MSG_STORAGE_LEVEL = xbt_lib_add_level(storage_lib, (void_f_pvoid_t) __MSG_storage_destroy);
  MSG_FILE_LEVEL = xbt_lib_add_level(file_lib, (void_f_pvoid_t) __MSG_file_destroy);
  if(sg_cfg_get_boolean("clean_atexit")) atexit(MSG_exit);
}

/** \ingroup msg_simulation
 * \brief Launch the MSG simulation
 */
msg_error_t MSG_main(void)
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

  TRACE_surf_resource_utilization_release();
  TRACE_end();
  free(msg_global);
  msg_global = NULL;
}

/** \ingroup msg_simulation
 * \brief A clock (in second).
 */
double MSG_get_clock(void)
{
  return SIMIX_get_clock();
}

unsigned long int MSG_get_sent_msg()
{
  return msg_global->sent_msg;
}
