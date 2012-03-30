/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/heap.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/ex.h"             /* ex_backtrace_display */
#include "mc/mc.h"

XBT_LOG_NEW_CATEGORY(simix, "All SIMIX categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix,
                                "Logging specific to SIMIX (kernel)");

smx_global_t simix_global = NULL;
static xbt_heap_t simix_timers = NULL;

static void* SIMIX_action_mallocator_new_f(void);
static void SIMIX_action_mallocator_free_f(void* action);
static void SIMIX_action_mallocator_reset_f(void* action);

extern void smx_ctx_raw_new_sr(void);

/* FIXME: Yeah, I'll do it in a portable maner one day [Mt] */
#include <signal.h>

int _surf_do_verbose_exit = 1;
static void _XBT_CALL inthandler(int ignored)
{
  if ( _surf_do_verbose_exit ) {
     XBT_INFO("CTRL-C pressed. Displaying status and bailing out");
     SIMIX_display_process_status();
  }
  else {
     XBT_INFO("CTRL-C pressed. bailing out without displaying because verbose-exit is disabled");
  }
  exit(1);
}

/********************************* SIMIX **************************************/

XBT_INLINE double SIMIX_timer_next(void)
{
  return xbt_heap_size(simix_timers) > 0 ? xbt_heap_maxkey(simix_timers) : -1.0;
}

/**
 * \brief Initialize SIMIX internal data.
 *
 * \param argc Argc
 * \param argv Argv
 */
void SIMIX_global_init(int *argc, char **argv)
{
  s_smx_process_t proc;

  if (!simix_global) {
    simix_global = xbt_new0(s_smx_global_t, 1);

    simix_global->process_to_run = xbt_dynar_new(sizeof(smx_process_t), NULL);
    simix_global->process_that_ran = xbt_dynar_new(sizeof(smx_process_t), NULL);
    simix_global->process_list =
        xbt_swag_new(xbt_swag_offset(proc, process_hookup));
    simix_global->process_to_destroy =
        xbt_swag_new(xbt_swag_offset(proc, destroy_hookup));

    simix_global->maestro_process = NULL;
    simix_global->registered_functions = xbt_dict_new_homogeneous(NULL);

    simix_global->create_process_function = SIMIX_process_create;
    simix_global->kill_process_function = SIMIX_process_kill;
    simix_global->cleanup_process_function = SIMIX_process_cleanup;
    simix_global->action_mallocator = xbt_mallocator_new(65536,
        SIMIX_action_mallocator_new_f, SIMIX_action_mallocator_free_f,
        SIMIX_action_mallocator_reset_f);

    surf_init(argc, argv);      /* Initialize SURF structures */
    SIMIX_context_mod_init();
    SIMIX_create_maestro_process();

    /* context exception handlers */
    __xbt_running_ctx_fetch = SIMIX_process_get_running_context;
    __xbt_ex_terminate = SIMIX_process_exception_terminate;

    /* Initialize the SIMIX network module */
    SIMIX_network_init();

    /* Prepare to display some more info when dying on Ctrl-C pressing */
    signal(SIGINT, inthandler);
  }
  if (!simix_timers) {
    simix_timers = xbt_heap_new(8, &free);
  }

  XBT_DEBUG("ADD SIMIX LEVELS");
  SIMIX_HOST_LEVEL = xbt_lib_add_level(host_lib,SIMIX_host_destroy);
}

/**
 * \brief Clean the SIMIX simulation
 *
 * This functions remove the memory used by SIMIX
 */
void SIMIX_clean(void)
{
#ifdef TIME_BENCH
  smx_ctx_raw_new_sr();
#endif

  /* Kill everyone (except maestro) */
  SIMIX_process_killall(simix_global->maestro_process);

  /* Exit the SIMIX network module */
  SIMIX_network_exit();

  xbt_heap_free(simix_timers);
  simix_timers = NULL;
  /* Free the remaining data structures */
  xbt_dynar_free(&simix_global->process_to_run);
  xbt_dynar_free(&simix_global->process_that_ran);
  xbt_swag_free(simix_global->process_to_destroy);
  xbt_swag_free(simix_global->process_list);
  simix_global->process_list = NULL;
  simix_global->process_to_destroy = NULL;
  xbt_dict_free(&(simix_global->registered_functions));

  /* Let's free maestro now */
  SIMIX_context_free(simix_global->maestro_process->context);
  xbt_free(simix_global->maestro_process->running_ctx);
  xbt_free(simix_global->maestro_process);
  simix_global->maestro_process = NULL;

  /* Restore the default exception setup */
  __xbt_running_ctx_fetch = &__xbt_ex_ctx_default;
  __xbt_ex_terminate = &__xbt_ex_terminate_default;

  /* Finish context module and SURF */
  SIMIX_context_mod_exit();

  surf_exit();

  xbt_mallocator_free(simix_global->action_mallocator);
  xbt_free(simix_global);
  simix_global = NULL;

  return;
}


/**
 * \brief A clock (in second).
 *
 * \return Return the clock.
 */
XBT_INLINE double SIMIX_get_clock(void)
{
  if(MC_IS_ENABLED){
    return MC_process_clock_get(SIMIX_process_self());
  }else{
    return surf_get_clock();
  }
}

void SIMIX_run(void)
{
  double time = 0;
  smx_process_t process;
  xbt_swag_t set;
  surf_action_t action;
  smx_timer_t timer;
  surf_model_t model;
  unsigned int iter;

  do {
    XBT_DEBUG("New Schedule Round; size(queue)=%lu",
        xbt_dynar_length(simix_global->process_to_run));
#ifdef TIME_BENCH
    smx_ctx_raw_new_sr();
#endif
    while (!xbt_dynar_is_empty(simix_global->process_to_run)) {
      XBT_DEBUG("New Sub-Schedule Round; size(queue)=%lu",
              xbt_dynar_length(simix_global->process_to_run));
      SIMIX_process_runall();
      xbt_dynar_foreach(simix_global->process_that_ran, iter, process) {
        if (process->simcall.call != SIMCALL_NONE) {
          SIMIX_simcall_pre(&process->simcall, 0);
        }
      }
    }

    time = SIMIX_timer_next();
    if (time != -1.0 || xbt_swag_size(simix_global->process_list) != 0)
      time = surf_solve(time);

    /* Notify all the hosts that have failed */
    /* FIXME: iterate through the list of failed host and mark each of them */
    /* as failed. On each host, signal all the running processes with host_fail */

    /* Handle any pending timer */
    while (xbt_heap_size(simix_timers) > 0 && SIMIX_get_clock() >= SIMIX_timer_next()) {
       //FIXME: make the timers being real callbacks
       // (i.e. provide dispatchers that read and expand the args) 
       timer = xbt_heap_pop(simix_timers);
       if (timer->func)
         ((void (*)(void*))timer->func)(timer->args);
    }
    /* Wake up all processes waiting for a Surf action to finish */
    xbt_dynar_foreach(model_list, iter, model) {
      set = model->states.failed_action_set;
      while ((action = xbt_swag_extract(set)))
        SIMIX_simcall_post((smx_action_t) action->data);
      set = model->states.done_action_set;
      while ((action = xbt_swag_extract(set)))
        SIMIX_simcall_post((smx_action_t) action->data);
    }

    /* Clean processes to destroy */
    SIMIX_process_empty_trash();

  } while (time != -1.0 || !xbt_dynar_is_empty(simix_global->process_to_run));

  if (xbt_swag_size(simix_global->process_list) != 0) {

#ifdef HAVE_TRACING
    TRACE_end();
#endif

    XBT_WARN("Oops ! Deadlock or code not perfectly clean.");
    SIMIX_display_process_status();
    xbt_abort();
  }
}

/**
 * 	\brief Set the date to execute a function
 *
 * Set the date to execute the function on the surf.
 * 	\param date Date to execute function
 * 	\param function Function to be executed
 * 	\param arg Parameters of the function
 *
 */
XBT_INLINE void SIMIX_timer_set(double date, void *function, void *arg)
{
  smx_timer_t timer = xbt_new0(s_smx_timer_t, 1);

  timer->date = date;
  timer->func = function;
  timer->args = arg;
  xbt_heap_push(simix_timers, timer, date);
}

/**
 * \brief Registers a function to create a process.
 *
 * This function registers a function to be called
 * when a new process is created. The function has
 * to call SIMIX_process_create().
 * \param function create process function
 */
XBT_INLINE void SIMIX_function_register_process_create(smx_creation_func_t
                                                       function)
{
  simix_global->create_process_function = function;
}

/**
 * \brief Registers a function to kill a process.
 *
 * This function registers a function to be called when a
 * process is killed. The function has to call the SIMIX_process_kill().
 *
 * \param function Kill process function
 */
XBT_INLINE void SIMIX_function_register_process_kill(void_pfn_smxprocess_t
                                                     function)
{
  simix_global->kill_process_function = function;
}

/**
 * \brief Registers a function to cleanup a process.
 *
 * This function registers a user function to be called when
 * a process ends properly.
 *
 * \param function cleanup process function
 */
XBT_INLINE void SIMIX_function_register_process_cleanup(void_pfn_smxprocess_t
                                                        function)
{
  simix_global->cleanup_process_function = function;
}


void SIMIX_display_process_status(void)
{
  if (simix_global->process_list == NULL) {
    return;
  }

  smx_process_t process = NULL;
  int nbprocess = xbt_swag_size(simix_global->process_list);

  XBT_INFO("%d processes are still running, waiting for something.", nbprocess);
  /*  List the process and their state */
  XBT_INFO
    ("Legend of the following listing: \"Process <pid> (<name>@<host>): <status>\"");
  xbt_swag_foreach(process, simix_global->process_list) {

    if (process->waiting_action) {

      const char* action_description = "unknown";
      switch (process->waiting_action->type) {

      case SIMIX_ACTION_EXECUTE:
        action_description = "execution";
        break;

      case SIMIX_ACTION_PARALLEL_EXECUTE:
        action_description = "parallel execution";
        break;

      case SIMIX_ACTION_COMMUNICATE:
        action_description = "communication";
        break;

      case SIMIX_ACTION_SLEEP:
        action_description = "sleeping";
        break;

      case SIMIX_ACTION_SYNCHRO:
        action_description = "synchronization";
        break;

      case SIMIX_ACTION_IO:
        action_description = "I/O";
        break;
      }
      XBT_INFO("Process %lu (%s@%s): waiting for %s action %p (%s) in state %d to finish",
          process->pid, process->name, process->smx_host->name,
          action_description, process->waiting_action,
          process->waiting_action->name, (int)process->waiting_action->state);
    }
    else {
      XBT_INFO("Process %lu (%s@%s)", process->pid, process->name, process->smx_host->name);
    }
  }
}

static void* SIMIX_action_mallocator_new_f(void) {
  smx_action_t action = xbt_new(s_smx_action_t, 1);
  action->simcalls = xbt_fifo_new();
  return action;
}

static void SIMIX_action_mallocator_free_f(void* action) {
  xbt_fifo_free(((smx_action_t) action)->simcalls);
  xbt_free(action);
}

static void SIMIX_action_mallocator_reset_f(void* action) {

  // we also recycle the simcall list
  xbt_fifo_t fifo = ((smx_action_t) action)->simcalls;
  xbt_fifo_reset(fifo);
  memset(action, 0, sizeof(s_smx_action_t));
  ((smx_action_t) action)->simcalls = fifo;
}
