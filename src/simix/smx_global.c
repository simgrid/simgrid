/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/heap.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/ex.h"             /* ex_backtrace_display */

XBT_LOG_EXTERNAL_CATEGORY(simix);
XBT_LOG_EXTERNAL_CATEGORY(simix_action);
XBT_LOG_EXTERNAL_CATEGORY(simix_deployment);
XBT_LOG_EXTERNAL_CATEGORY(simix_environment);
XBT_LOG_EXTERNAL_CATEGORY(simix_host);
XBT_LOG_EXTERNAL_CATEGORY(simix_process);
XBT_LOG_EXTERNAL_CATEGORY(simix_synchro);
XBT_LOG_EXTERNAL_CATEGORY(simix_context);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix,
                                "Logging specific to SIMIX (kernel)");

smx_global_t simix_global = NULL;
static xbt_heap_t simix_timers = NULL;

/* FIXME: Yeah, I'll do it in a portable maner one day [Mt] */
#include <signal.h>

static void _XBT_CALL inthandler(int ignored)
{
  INFO0("CTRL-C pressed. Displaying status and bailing out");
  SIMIX_display_process_status();
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
    /* Connect our log channels: that must be done manually under windows */
    XBT_LOG_CONNECT(simix_action, simix);
    XBT_LOG_CONNECT(simix_deployment, simix);
    XBT_LOG_CONNECT(simix_environment, simix);
    XBT_LOG_CONNECT(simix_host, simix);
    XBT_LOG_CONNECT(simix_kernel, simix);
    XBT_LOG_CONNECT(simix_process, simix);
    XBT_LOG_CONNECT(simix_synchro, simix);
    XBT_LOG_CONNECT(simix_context, simix);

    simix_global = xbt_new0(s_smx_global_t, 1);

    simix_global->host = xbt_dict_new();
    simix_global->process_to_run = xbt_dynar_new(sizeof(void *), NULL);
    simix_global->process_list =
        xbt_swag_new(xbt_swag_offset(proc, process_hookup));
    simix_global->process_to_destroy =
        xbt_swag_new(xbt_swag_offset(proc, destroy_hookup));

    simix_global->maestro_process = NULL;
    simix_global->registered_functions = xbt_dict_new();

    simix_global->create_process_function = NULL;
    simix_global->kill_process_function = NULL;
    simix_global->cleanup_process_function = SIMIX_process_cleanup;

    surf_init(argc, argv);      /* Initialize SURF structures */
    SIMIX_context_mod_init();
    SIMIX_create_maestro_process();

    /* context exception handlers */
    __xbt_running_ctx_fetch = SIMIX_process_get_running_context;
    __xbt_ex_terminate = SIMIX_process_exception_terminate;

    /* Initialize request mechanism */
    SIMIX_request_init();

    /* Initialize the SIMIX network module */
    SIMIX_network_init();

    /* Prepare to display some more info when dying on Ctrl-C pressing */
    signal(SIGINT, inthandler);
  }
  if (!simix_timers) {
    simix_timers = xbt_heap_new(8, &free);
  }
}

/**
 * \brief Clean the SIMIX simulation
 *
 * This functions remove the memory used by SIMIX
 */
void SIMIX_clean(void)
{
  /* Kill everyone (except maestro) */
  SIMIX_process_killall();

  /* Exit the SIMIX network module */
  SIMIX_network_exit();

  /* Exit request mechanism */
  SIMIX_request_destroy();

  xbt_heap_free(simix_timers);
  /* Free the remaining data structures */
  xbt_dynar_free(&simix_global->process_to_run);
  xbt_swag_free(simix_global->process_to_destroy);
  xbt_swag_free(simix_global->process_list);
  simix_global->process_list = NULL;
  simix_global->process_to_destroy = NULL;
  xbt_dict_free(&(simix_global->registered_functions));
  xbt_dict_free(&(simix_global->host));

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
  return surf_get_clock();
}

void SIMIX_run(void)
{
  double time = 0;
  smx_req_t req;
  xbt_swag_t set;
  surf_action_t action;
  smx_timer_t timer;
  surf_model_t model;
  unsigned int iter;

  do {
    do {
      DEBUG1("New Schedule Round; size(queue)=%lu",
          xbt_dynar_length(simix_global->process_to_run));
      SIMIX_context_runall(simix_global->process_to_run);
      while ((req = SIMIX_request_pop())) {
        DEBUG1("Handling request %p", req);
        SIMIX_request_pre(req, 0);
      }
    } while (xbt_dynar_length(simix_global->process_to_run));

    time = surf_solve(SIMIX_timer_next());

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
    /* Wake up all process waiting for the action finish */
    xbt_dynar_foreach(model_list, iter, model) {
      for (set = model->states.failed_action_set;
           set;
           set = (set == model->states.failed_action_set)
                 ? model->states.done_action_set
                 : NULL) {
        while ((action = xbt_swag_extract(set)))
          SIMIX_request_post((smx_action_t) action->data);
      }
    }
  } while (time != -1.0);

  if (xbt_swag_size(simix_global->process_list) != 0) {

    WARN0("Oops ! Deadlock or code not perfectly clean.");
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
 *	\brief Registers a function to create a process.
 *
 *	This function registers an user function to be called when a new process is created. The user function have to call the SIMIX_create_process function.
 *	\param function Create process function
 *
 */
XBT_INLINE void SIMIX_function_register_process_create(smx_creation_func_t
                                                       function)
{
  xbt_assert0((simix_global->create_process_function == NULL),
              "Data already set");

  simix_global->create_process_function = function;
}

/**
 *	\brief Registers a function to kill a process.
 *
 *	This function registers an user function to be called when a new process is killed. The user function have to call the SIMIX_kill_process function.
 *	\param function Kill process function
 *
 */
XBT_INLINE void SIMIX_function_register_process_kill(void_f_pvoid_t
                                                     function)
{
  xbt_assert0((simix_global->kill_process_function == NULL),
              "Data already set");

  simix_global->kill_process_function = function;
}

/**
 *	\brief Registers a function to cleanup a process.
 *
 *	This function registers an user function to be called when a new process ends properly.
 *	\param function cleanup process function
 *
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

  INFO1("%d processes are still running, waiting for something.", nbprocess);
  /*  List the process and their state */
  INFO0
    ("Legend of the following listing: \"<process> on <host>: <status>.\"");
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
      INFO2("Waiting for %s action %p to finish", action_description, process->waiting_action);
    }
  }
}
