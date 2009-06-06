/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/ex.h"             /* ex_backtrace_display */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix,
                                "Logging specific to SIMIX (kernel)");

SIMIX_Global_t simix_global = NULL;

/********************************* SIMIX **************************************/
static void __simix_config_helper(const char *name, ...)
{
  va_list pa;
  va_start(pa, name);

  SIMIX_config(name, pa);

  va_end(pa);
}

static void simix_cfg_control_set(const char *control_string)
{
  /* To split the string in commands, and the cursors */
  xbt_dynar_t set_strings;
  char *str;
  unsigned int cpt;

  if (!control_string)
    return;
  DEBUG1("Parse log settings '%s'", control_string);

  /* split the string, and remove empty entries */
  set_strings = xbt_str_split_quoted(control_string);

  if (xbt_dynar_length(set_strings) == 0) {     /* vicious user! */
    xbt_dynar_free(&set_strings);
    return;
  }
  /* Parse each entry and either use it right now (if the category was already
     created), or store it for further use */
  xbt_dynar_foreach(set_strings, cpt, str) {
    char *control_string, *control_string_sav, *name, *value;


    control_string = control_string_sav = strdup(str);
    control_string += strspn(control_string, " ");
    name = control_string;
    control_string += strcspn(str, ":=");
    value = control_string;
    *value = 0;
    value++;

    xbt_assert1(strlen(name) != 0, "Invalid name for configuration: '%s'",
                name);
    xbt_assert1(strlen(value) != 0,
                "Invalid value for configuration: '%s'", value);
    INFO2("setting '%s' to '%s'", name, value);

    __simix_config_helper(name, value);

    free(control_string_sav);
  }
  xbt_dynar_free(&set_strings);
}

static void simix_cfg_init(int *argc, char **argv)
{
  int i, j;
  char *opt;

  for (i = 1; i < *argc; i++) {
    if (!strncmp(argv[i], "--cfg=", strlen("--cfg="))) {
      opt = strchr(argv[i], '=');
      opt++;

      simix_cfg_control_set(opt);
      DEBUG1("Did apply '%s' as config setting", opt);
      /*remove this from argv */

      for (j = i + 1; j < *argc; j++) {
        argv[j - 1] = argv[j];
      }

      argv[j - 1] = NULL;
      (*argc)--;
      i--;                      /* compensate effect of next loop incrementation */
    }
  }
}

/* FIXME: Yeah, I'll do it in a portable maner one day [Mt] */
#include <signal.h>

static void _XBT_CALL inthandler(int ignored)
{
  INFO0("CTRL-C pressed. Displaying status and bailing out");
  SIMIX_display_process_status();
  exit(1);
}


/**
 * \brief Initialize some SIMIX internal data.
 *
 * \param argc Argc
 * \param argv Argv
 */
void SIMIX_global_init(int *argc, char **argv)
{
  s_smx_process_t proc;

  if (!simix_global) {
    surf_init(argc, argv);      /* Initialize some common structures. Warning, it sets simix_global=NULL */
    simix_cfg_init(argc, argv);

    simix_global = xbt_new0(s_SIMIX_Global_t, 1);

    simix_global->host = xbt_dict_new();
    simix_global->process_to_run =
      xbt_swag_new(xbt_swag_offset(proc, synchro_hookup));
    simix_global->process_list =
      xbt_swag_new(xbt_swag_offset(proc, process_hookup));
    simix_global->current_process = NULL;
    simix_global->registered_functions = xbt_dict_new();

    simix_global->create_process_function = NULL;
    simix_global->kill_process_function = NULL;
    simix_global->cleanup_process_function = SIMIX_process_cleanup;

    /* Prepare to display some more info when dying on Ctrl-C pressing */
    signal(SIGINT, inthandler);
  }
}

/* Debug purpose, incomplete */
void SIMIX_display_process_status(void)
{
  smx_process_t process = NULL;
  xbt_fifo_item_t item = NULL;
  smx_action_t act;
  int nbprocess = xbt_swag_size(simix_global->process_list);

  INFO1("%d processes are still running, waiting for something.", nbprocess);
  /*  List the process and their state */
  INFO0
    ("Legend of the following listing: \"<process> on <host>: <status>.\"");
  xbt_swag_foreach(process, simix_global->process_list) {
    smx_simdata_process_t p_simdata =
      (smx_simdata_process_t) process->simdata;
    // simdata_host_t h_simdata=(simdata_host_t)p_simdata->host->simdata;
    char *who, *who2;

    asprintf(&who, "%s on %s: %s",
             process->name,
             p_simdata->smx_host->name,
             (process->simdata->blocked) ? "[BLOCKED] "
             : ((process->simdata->suspended) ? "[SUSPENDED] " : ""));

    if (p_simdata->mutex) {
      who2 =
        bprintf("%s Blocked on mutex %p", who,
                (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose)) ?
                p_simdata->mutex : (void *) 0xdead);
      free(who);
      who = who2;
    } else if (p_simdata->cond) {
      who2 =
        bprintf
        ("%s Blocked on condition %p; Waiting for the following actions:",
         who,
         (XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_verbose)) ?
         p_simdata->cond : (void *) 0xdead);
      free(who);
      who = who2;
      xbt_fifo_foreach(p_simdata->cond->actions, item, act, smx_action_t) {
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
    INFO1("%s.", who);
    free(who);
  }
}


/**
 * \brief Launch the SIMIX simulation, debug purpose
 */
void __SIMIX_main(void)
{
  smx_process_t process = NULL;
  smx_cond_t cond = NULL;
  smx_action_t smx_action;
  xbt_fifo_t actions_done = xbt_fifo_new();
  xbt_fifo_t actions_failed = xbt_fifo_new();

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  //surf_solve(); /* Takes traces into account. Returns 0.0 */
  /* xbt_fifo_size(msg_global->process_to_run) */

  while (SIMIX_solve(actions_done, actions_failed) != -1.0) {

    while ((smx_action = xbt_fifo_pop(actions_failed))) {

      xbt_fifo_item_t _cursor;

      DEBUG1("** %s failed **", smx_action->name);
      xbt_fifo_foreach(smx_action->cond_list, _cursor, cond, smx_cond_t) {
        xbt_swag_foreach(process, cond->sleeping) {
          DEBUG2("\t preparing to wake up %s on %s",
                 process->name, process->simdata->smx_host->name);
        }
        SIMIX_cond_broadcast(cond);
        /* remove conditional from action */
        SIMIX_unregister_action_to_condition(smx_action, cond);
      }
    }

    while ((smx_action = xbt_fifo_pop(actions_done))) {
      xbt_fifo_item_t _cursor;

      DEBUG1("** %s done **", smx_action->name);
      xbt_fifo_foreach(smx_action->cond_list, _cursor, cond, smx_cond_t) {
        xbt_swag_foreach(process, cond->sleeping) {
          DEBUG2("\t preparing to wake up %s on %s",
                 process->name, process->simdata->smx_host->name);
        }
        SIMIX_cond_broadcast(cond);
        /* remove conditional from action */
        SIMIX_unregister_action_to_condition(smx_action, cond);
      }
    }
  }
  return;
}

/**
 * \brief Kill all running process
 *
 */
void SIMIX_process_killall()
{
  smx_process_t p = NULL;
  smx_process_t self = SIMIX_process_self();

  while ((p = xbt_swag_extract(simix_global->process_list))) {
    if (p != self)
      SIMIX_process_kill(p);
  }

  xbt_context_empty_trash();

  if (self) {
    xbt_context_yield();
  }

  return;
}

/**
 * \brief Clean the SIMIX simulation
 *
 * This functions remove all memories needed to the SIMIX execution
 */
void SIMIX_clean(void)
{
  smx_process_t p = NULL;

  while ((p = xbt_swag_extract(simix_global->process_list))) {
    SIMIX_process_kill(p);
  }

  xbt_dict_free(&(simix_global->host));
  xbt_swag_free(simix_global->process_to_run);
  xbt_swag_free(simix_global->process_list);
  xbt_dict_free(&(simix_global->registered_functions));
  simix_config_finalize();
  free(simix_global);
  simix_global = NULL;

  surf_exit();

  return;
}


/**
 * \brief A clock (in second).
 *
 * \return Return the clock.
 */
double SIMIX_get_clock(void)
{
  return surf_get_clock();
}

/**
 * 	\brief Finish the simulation initialization
 *
 *      Must be called before the first call to SIMIX_solve()
 */
void SIMIX_init(void)
{
  surf_presolve();
}

/**
 * 	\brief Does a turn of the simulation
 *
 *	Executes a step in the surf simulation, adding to the two lists all the actions that finished on this turn. Schedules all processus in the process_to_run list.
 * 	\param actions_done List of actions done
 * 	\param actions_failed List of actions failed
 * 	\return The time spent to execute the simulation or -1 if the simulation ended
 */
double SIMIX_solve(xbt_fifo_t actions_done, xbt_fifo_t actions_failed)
{

  smx_process_t process = NULL;
  unsigned int iter;
  double elapsed_time = 0.0;
  static int state_modifications = 1;

  xbt_context_empty_trash();
  if (xbt_swag_size(simix_global->process_to_run) && (elapsed_time > 0)) {
    DEBUG0("**************************************************");
  }

  while ((process = xbt_swag_extract(simix_global->process_to_run))) {
    DEBUG2("Scheduling %s on %s",
           process->name, process->simdata->smx_host->name);
    simix_global->current_process = process;
    xbt_context_schedule(process->simdata->context);
    /*       fflush(NULL); */
    simix_global->current_process = NULL;
  }

  {
    surf_action_t action = NULL;
    surf_model_t model = NULL;
    smx_action_t smx_action = NULL;

    void *fun = NULL;
    void *arg = NULL;

    xbt_dynar_foreach(model_list, iter, model) {
      if (xbt_swag_size(model->common_public->states.failed_action_set)
          || xbt_swag_size(model->common_public->states.done_action_set)) {
        state_modifications = 1;
        break;
      }
    }

    if (!state_modifications) {
      DEBUG1("%f : Calling surf_solve", SIMIX_get_clock());
      elapsed_time = surf_solve();
      DEBUG1("Elapsed_time %f", elapsed_time);
    }

    while (surf_timer_model->extension_public->get(&fun, (void *) &arg)) {
      DEBUG2("got %p %p", fun, arg);
      if (fun == SIMIX_process_create) {
        smx_process_arg_t args = arg;
        DEBUG2("Launching %s on %s", args->name, args->hostname);
        process = SIMIX_process_create(args->name, args->code,
                                       args->data, args->hostname,
                                       args->argc, args->argv,
                                       args->properties);
        if (process && args->kill_time > SIMIX_get_clock()) {
          surf_timer_model->extension_public->set(args->kill_time, (void *)
                                                  &SIMIX_process_kill,
                                                  (void *) process);
        }
        xbt_free(args);
      }
      if (fun == SIMIX_process_kill) {
        process = arg;
        DEBUG2("Killing %s on %s", process->name,
               process->simdata->smx_host->name);
        SIMIX_process_kill(process);
      }
    }

    /* Wake up all process waiting for the action finish */
    xbt_dynar_foreach(model_list, iter, model) {
      while ((action =
              xbt_swag_extract(model->common_public->
                               states.failed_action_set))) {
        smx_action = action->data;
        if (smx_action) {
          xbt_fifo_unshift(actions_failed, smx_action);
        }
      }
      while ((action =
              xbt_swag_extract(model->common_public->
                               states.done_action_set))) {
        smx_action = action->data;
        if (smx_action) {
          xbt_fifo_unshift(actions_done, smx_action);
        }
      }
    }
  }
  state_modifications = 0;

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
  return elapsed_time;
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
void SIMIX_timer_set(double date, void *function, void *arg)
{
  surf_timer_model->extension_public->set(date, function, arg);
}

int SIMIX_timer_get(void **function, void **arg)
{
  return surf_timer_model->extension_public->get(function, arg);
}

/**
 *	\brief Registers a function to create a process.
 *
 *	This function registers an user function to be called when a new process is created. The user function have to call the SIMIX_create_process function.
 *	\param function Create process function
 *
 */
void SIMIX_function_register_process_create(smx_creation_func_t function)
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
void SIMIX_function_register_process_kill(void_f_pvoid_t function)
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
void SIMIX_function_register_process_cleanup(void_f_pvoid_t function)
{
  simix_global->cleanup_process_function = function;
}
