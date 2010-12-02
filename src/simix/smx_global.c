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

SIMIX_Global_t simix_global = NULL;
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

    simix_global = xbt_new0(s_SIMIX_Global_t, 1);

    simix_global->host = xbt_dict_new();
    simix_global->process_to_run =
        xbt_swag_new(xbt_swag_offset(proc, synchro_hookup));
    simix_global->process_list =
        xbt_swag_new(xbt_swag_offset(proc, process_hookup));
    simix_global->process_to_destroy =
        xbt_swag_new(xbt_swag_offset(proc, destroy_hookup));

    simix_global->current_process = NULL;
    simix_global->maestro_process = NULL;
    simix_global->registered_functions = xbt_dict_new();

    simix_global->create_process_function = NULL;
    simix_global->kill_process_function = NULL;
    simix_global->cleanup_process_function = SIMIX_process_cleanup;

#ifdef HAVE_LATENCY_BOUND_TRACKING
    simix_global->latency_limited_dict = xbt_dict_new();
#endif

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
    surf_init(argc, argv);      /* Initialize SURF structures */
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
  xbt_swag_free(simix_global->process_to_run);
  xbt_swag_free(simix_global->process_to_destroy);
  xbt_swag_free(simix_global->process_list);
  simix_global->process_list = NULL;
  simix_global->process_to_destroy = NULL;
  xbt_dict_free(&(simix_global->registered_functions));
  xbt_dict_free(&(simix_global->host));

#ifdef HAVE_LATENCY_BOUND_TRACKING
  xbt_dict_free(&(simix_global->latency_limited_dict));
#endif

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
      DEBUG0("New Schedule Round");
      SIMIX_context_runall(simix_global->process_to_run);
      while((req = SIMIX_request_pop())){
        DEBUG1("Handling request %p", req);
        SIMIX_request_pre(req);
      }
    } while (xbt_swag_size(simix_global->process_to_run));

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
      for(set = model->states.failed_action_set;
          set;
          set = (set == model->states.failed_action_set)
                ? model->states.done_action_set
                : NULL) {
        while ((action = xbt_swag_extract(set)))
          SIMIX_request_post((smx_action_t)action->data);
      }
    }
  } while(time != -1.0);

}


/**
 * 	\brief Does a turn of the simulation
 *
 *	Executes a step in the surf simulation, adding to the two lists all the actions that finished on this turn. Schedules all processus in the process_to_run list.
 * 	\param actions_done List of actions done
 * 	\param actions_failed List of actions failed
 * 	\return The time spent to execute the simulation or -1 if the simulation ended
 */
/* FIXME: this function is now deprecated, remove it */
#if 0
double SIMIX_solve(xbt_fifo_t actions_done, xbt_fifo_t actions_failed)
{

  smx_process_t process = NULL;
  unsigned int iter;
  double elapsed_time = 0.0;
  static int state_modifications = 1;
  int actions_on_system = 0;
  smx_timer_t timer;

  SIMIX_process_empty_trash();
  if (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_debug) &&
      xbt_swag_size(simix_global->process_to_run) && (elapsed_time > 0)) {
    DEBUG0("**************************************************");
  }

  while ((process = xbt_swag_extract(simix_global->process_to_run))) {
    DEBUG2("Scheduling %s on %s", process->name, process->smx_host->name);
    /*SIMIX_process_schedule(process);*/
  }

  {
    surf_action_t action = NULL;
    surf_model_t model = NULL;
    smx_action_t smx_action = NULL;

    void *fun = NULL;
    void *arg = NULL;

    xbt_dynar_foreach(model_list, iter, model) {
      if (xbt_swag_size(model->states.failed_action_set)
          || xbt_swag_size(model->states.done_action_set)) {
        state_modifications = 1;
        break;
      }
      if (xbt_swag_size(model->states.running_action_set)
          || xbt_swag_size(model->states.ready_action_set)) {
        actions_on_system = 1;
      }
    }
    if (xbt_heap_size(simix_timers) > 0) {
      actions_on_system = 1;
    }

    /* only calls surf_solve if there are actions to run */
    if (!state_modifications && actions_on_system) {
      DEBUG1("Calling surf_solve(%f)", SIMIX_timer_next());
      elapsed_time = surf_solve(SIMIX_timer_next());
      DEBUG1("Elapsed time %f", elapsed_time);
    }

    actions_on_system = 0;
    while (xbt_heap_size(simix_timers) > 0 && SIMIX_get_clock() >= SIMIX_timer_next()) {
      timer = xbt_heap_pop(simix_timers);
      fun = timer->func;
      arg = timer->args;
      free(timer);
      /* change in process, don't quit */
      actions_on_system = 1;
      DEBUG3("got %p %p at %f", fun, arg, timer->date);
      if (fun == SIMIX_process_create) {
        smx_process_arg_t args = arg;
        DEBUG2("Launching %s on %s", args->name, args->hostname);
        process = SIMIX_process_create(args->name, args->code,
                                       args->data, args->hostname,
                                       args->argc, args->argv,
                                       args->properties);
        /* verify if process has been created */
        if (!process) {
          xbt_free(args);
          continue;
        }

        if (args->kill_time > SIMIX_get_clock()) {
          SIMIX_timer_set(args->kill_time, &SIMIX_process_kill, process);
        }
        xbt_free(args);
      } else if (fun == simix_global->create_process_function) {
        smx_process_arg_t args = arg;
        DEBUG2("Launching %s on %s", args->name, args->hostname);
        process =
            (*simix_global->create_process_function) (args->name,
                                                      args->code,
                                                      args->data,
                                                      args->hostname,
                                                      args->argc,
                                                      args->argv,
                                                      args->properties);
        /* verify if process has been created */
        if (!process) {
          xbt_free(args);
          continue;
        }
        if (args->kill_time > SIMIX_get_clock()) {
          if (simix_global->kill_process_function)
            SIMIX_timer_set(args->kill_time, simix_global->kill_process_function, process);
          else
            SIMIX_timer_set(args->kill_time, &SIMIX_process_kill, process);
        }
        xbt_free(args);
      } else if (fun == SIMIX_process_kill) {
        process = arg;
        DEBUG2("Killing %s on %s", process->name, process->smx_host->name);
        SIMIX_process_kill(process, SIMIX_process_self());
      } else if (fun == simix_global->kill_process_function) {
        process = arg;
        (*simix_global->kill_process_function) (process);
      } else {
        //FIXME: ((void (*)(void*))fun)(arg);
        THROW_IMPOSSIBLE;
      }
    }

    /* Wake up all process waiting for the action finish */
    xbt_dynar_foreach(model_list, iter, model) {
      /* stop simulation case there are no actions to run */
      if ((xbt_swag_size(model->states.running_action_set)) ||
          (xbt_swag_size(model->states.ready_action_set)) ||
          (xbt_swag_size(model->states.done_action_set)) ||
          (xbt_swag_size(model->states.failed_action_set)))
        actions_on_system = 1;

      while ((action = xbt_swag_extract(model->states.failed_action_set))) {
        smx_action = action->data;
        if (smx_action) {
//          SIMIX_action_signal_all(smx_action);
        }
      }
      while ((action = xbt_swag_extract(model->states.done_action_set))) {
        smx_action = action->data;
        if (smx_action) {
//          SIMIX_action_signal_all(smx_action);
        }
      }
    }
  }

  if (xbt_heap_size(simix_timers) > 0) {
    actions_on_system = 1;
  }

  state_modifications = 0;
  if (!actions_on_system)
    elapsed_time = -1;

  if (elapsed_time == -1) {
    if (xbt_swag_size(simix_global->process_list) == 0) {
/* 			INFO0("Congratulations ! Simulation terminated : all processes are over"); */
    } else {
      INFO0("Oops ! Deadlock or code not perfectly clean.");
      SIMIX_display_process_status();
      if (XBT_LOG_ISENABLED(simix, xbt_log_priority_debug) ||
          XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_debug)) {
        DEBUG0("Aborting!");
        xbt_abort();
      }
      INFO0("Return a Warning.");
    }
  }

  DEBUG1("SIMIX_solve() finished, elapsed_time = %f", elapsed_time);
  return elapsed_time;
}
#endif

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

XBT_INLINE double SIMIX_timer_next(void)
{
  return xbt_heap_size(simix_timers) > 0 ? xbt_heap_maxkey(simix_timers) : -1.0;
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
  /*xbt_fifo_item_t item = NULL;
  smx_action_t act;*/
  int nbprocess = xbt_swag_size(simix_global->process_list);

  INFO1("%d processes are still running, waiting for something.", nbprocess);
  /*  List the process and their state */
  INFO0
    ("Legend of the following listing: \"<process> on <host>: <status>.\"");
  xbt_swag_foreach(process, simix_global->process_list) {
    char *who, *who2;

    asprintf(&who, "%s on %s: %s",
             process->name,
             process->smx_host->name,
             (process->blocked) ? "[BLOCKED] "
             : ((process->suspended) ? "[SUSPENDED] " : ""));

    if (process->waiting_action) {
      who2 = bprintf("Waiting for action %p to finish", process->waiting_action);
    }

      /*
    if (process->mutex) {
      who2 =
        bprintf("%s Blocked on mutex %p", who,
                (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose)) ?
                process->mutex : (void *) 0xdead);
      free(who);
      who = who2;
    } else if (process->cond) {
      who2 =
        bprintf
        ("%s Blocked on condition %p; Waiting for the following actions:",
         who,
         (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose)) ?
         process->cond : (void *) 0xdead);
      free(who);
      who = who2;
      xbt_fifo_foreach(process->cond->actions, item, act, smx_action_t) {
        who2 =
          bprintf("%s '%s'(%p)", who, act->name,
                  (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose))
                  ? act : (void *) 0xdead);
        free(who);
        who = who2;
      }
    } else if (process->sem) {
      who2 =
        bprintf
        ("%s Blocked on semaphore %p; Waiting for the following actions:",
         who,
         (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose)) ?
         process->sem : (void *) 0xdead);
      free(who);
      who = who2;
      xbt_fifo_foreach(process->sem->actions, item, act, smx_action_t) {
        who2 =
          bprintf("%s '%s'(%p)", who, act->name,
                  (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose))
                  ? act : (void *) 0xdead);
        free(who);
        who = who2;
      }

    } else {
      who2 =
        bprintf
        ("%s Blocked in an unknown status (please report this bug)", who);
      free(who);
      who = who2;
    }
    */
    INFO1("%s.", who);
    free(who);
  }
}
