/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h" /* ex_backtrace_display */
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_kernel, simix,
				"Logging specific to SIMIX (kernel)");


SIMIX_Global_t simix_global = NULL;


/** \defgroup simix_simulation   SIMIX simulation Functions
 *  \brief This section describes the functions you need to know to
 *  set up a simulation. You should have a look at \ref SIMIX_examples 
 *  to have an overview of their usage.
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Simulation functions" --> \endhtmlonly
 */

/********************************* SIMIX **************************************/

/** \ingroup simix_simulation
 * \brief Initialize some SIMIX internal data.
 */
void SIMIX_global_init_args(int *argc, char **argv)
{
  SIMIX_global_init(argc,argv);
}

/** \ingroup simix_simulation
 * \brief Initialize some SIMIX internal data.
 */
void SIMIX_global_init(int *argc, char **argv)
{
	s_smx_process_t proc;

	if (!simix_global) {
		surf_init(argc, argv);	/* Initialize some common structures. Warning, it sets simix_global=NULL */

		simix_global = xbt_new0(s_SIMIX_Global_t,1);

		simix_global->host = xbt_fifo_new();
		simix_global->process_to_run = xbt_swag_new(xbt_swag_offset(proc,synchro_hookup));
		simix_global->process_list = xbt_swag_new(xbt_swag_offset(proc,process_hookup));
		simix_global->current_process = NULL;
		simix_global->registered_functions = xbt_dict_new();
	}
}

/* Debug purpose, incomplete */
void __SIMIX_display_process_status(void)
{
   smx_process_t process = NULL;
   xbt_fifo_item_t item = NULL;
	 smx_action_t act;
   int nbprocess=xbt_swag_size(simix_global->process_list);
   
   INFO1("SIMIX: %d processes are still running, waiting for something.",
	 nbprocess);
   /*  List the process and their state */
   INFO0("SIMIX: <process> on <host>: <status>.");
   xbt_swag_foreach(process, simix_global->process_list) {
      smx_simdata_process_t p_simdata = (smx_simdata_process_t) process->simdata;
     // simdata_host_t h_simdata=(simdata_host_t)p_simdata->host->simdata;
      char *who;
	
      asprintf(&who,"SIMIX:  %s on %s: %s",
	       process->name,
				p_simdata->host->name,
	       (process->simdata->blocked)?"[BLOCKED] "
	       :((process->simdata->suspended)?"[SUSPENDED] ":""));
			if (p_simdata->mutex) {
				DEBUG1("Block on a mutex: %s", who);			
			}
			else if (p_simdata->cond) {
				DEBUG1("Block on a condition: %s", who);
				DEBUG0("Waiting actions:");
				xbt_fifo_foreach(p_simdata->cond->actions,item, act, smx_action_t) {
					DEBUG1("\t %s", act->name);
				}
			}
			else DEBUG1("Unknown block status: %s", who);
      free(who);
   }
}

/* FIXME: Yeah, I'll do it in a portable maner one day [Mt] */
#include <signal.h>

static void _XBT_CALL inthandler(int ignored)
{
   INFO0("CTRL-C pressed. Displaying status and bailing out");
   __SIMIX_display_process_status();
   exit(1);
}

/** \ingroup msg_simulation
 * \brief Launch the SIMIX simulation
 */
void __SIMIX_main(void)
{
	smx_process_t process = NULL;
	smx_cond_t cond = NULL;
	smx_action_t smx_action;
	xbt_fifo_t actions_done = xbt_fifo_new();
	xbt_fifo_t actions_failed = xbt_fifo_new();

	/* Prepare to display some more info when dying on Ctrl-C pressing */
	signal(SIGINT,inthandler);

	/* Clean IO before the run */
	fflush(stdout);
	fflush(stderr);

	//surf_solve(); /* Takes traces into account. Returns 0.0 */
	/* xbt_fifo_size(msg_global->process_to_run) */

	while (SIMIX_solve(actions_done, actions_failed) != -1.0) {

		while ( (smx_action = xbt_fifo_pop(actions_failed)) ) {

			xbt_fifo_item_t _cursor;

			DEBUG1("** %s failed **",smx_action->name);
			xbt_fifo_foreach(smx_action->cond_list,_cursor,cond,smx_cond_t) {
				xbt_swag_foreach(process,cond->sleeping) {
					DEBUG2("\t preparing to wake up %s on %s",	     
							process->name,	process->simdata->host->name);
				}
				SIMIX_cond_broadcast(cond);
				/* remove conditional from action */
				xbt_fifo_remove(smx_action->cond_list,cond);
			}
		}

		while ( (smx_action = xbt_fifo_pop(actions_done)) ) {
			xbt_fifo_item_t _cursor;

			DEBUG1("** %s done **",smx_action->name);
			xbt_fifo_foreach(smx_action->cond_list,_cursor,cond,smx_cond_t) {
				xbt_swag_foreach(process,cond->sleeping) {
					DEBUG2("\t preparing to wake up %s on %s",	     
							process->name,	process->simdata->host->name);
				}
				SIMIX_cond_broadcast(cond);
				/* remove conditional from action */
				xbt_fifo_remove(smx_action->cond_list,cond);
			}
		}
	}
	return;
}

/** \ingroup msg_simulation
 * \brief Kill all running process

 * \param reset_PIDs should we reset the PID numbers. A negative
 *   number means no reset and a positive number will be used to set the PID
 *   of the next newly created process.
 */
void SIMIX_process_killall()
{
  smx_process_t p = NULL;
  smx_process_t self = SIMIX_process_self();

  while((p=xbt_swag_extract(simix_global->process_list))) {
    if(p!=self) SIMIX_process_kill(p);
  }

  xbt_context_empty_trash();

  if(self) {
    xbt_context_yield();
  }

  return;
}

/** \ingroup msg_simulation
 * \brief Clean the SIMIX simulation
 */
void SIMIX_clean(void)
{
  xbt_fifo_item_t i = NULL;
  smx_host_t h = NULL;
  smx_process_t p = NULL;


  while((p=xbt_swag_extract(simix_global->process_list))) {
    SIMIX_process_kill(p);
  }

  xbt_fifo_foreach(simix_global->host,i,h,smx_host_t) {
    __SIMIX_host_destroy(h);
  }
  xbt_fifo_free(simix_global->host);
  xbt_swag_free(simix_global->process_to_run);
  xbt_swag_free(simix_global->process_list);
  xbt_dict_free(&(simix_global->registered_functions));
  simix_config_finalize();
  free(simix_global);
  surf_exit();

  return ;
}


/** \ingroup msg_easier_life
 * \brief A clock (in second).
 */
double SIMIX_get_clock(void)
{
  return surf_get_clock();
}

double SIMIX_solve(xbt_fifo_t actions_done, xbt_fifo_t actions_failed) 
{

	smx_process_t process = NULL;
	int i;
	double elapsed_time = 0.0;
	int state_modifications = 0;


	//surf_solve(); /* Takes traces into account. Returns 0.0  I don't know what to do with this*/

	xbt_context_empty_trash();
	if(xbt_swag_size(simix_global->process_to_run) && (elapsed_time>0)) {
		DEBUG0("**************************************************");
	}

	while ((process = xbt_swag_extract(simix_global->process_to_run))) {
		DEBUG2("Scheduling %s on %s",	     
				process->name,
				process->simdata->host->name);
		simix_global->current_process = process;
		xbt_context_schedule(process->simdata->context);
		/*       fflush(NULL); */
		simix_global->current_process = NULL;
	}

	{
		surf_action_t action = NULL;
		surf_resource_t resource = NULL;
		smx_action_t smx_action = NULL;

		void *fun = NULL;
		void *arg = NULL;

		xbt_dynar_foreach(resource_list, i, resource) {
			if(xbt_swag_size(resource->common_public->states.failed_action_set) ||
					xbt_swag_size(resource->common_public->states.done_action_set))
				state_modifications = 1;
		}

		if(!state_modifications) {
			DEBUG1("%f : Calling surf_solve",SIMIX_get_clock());
			elapsed_time = surf_solve();
			DEBUG1("Elapsed_time %f",elapsed_time);
		}

		while (surf_timer_resource->extension_public->get(&fun,(void*)&arg)) {
			DEBUG2("got %p %p", fun, arg);
			if(fun==SIMIX_process_create_with_arguments) {
				process_arg_t args = arg;
				DEBUG2("Launching %s on %s", args->name, args->host->name);
				process = SIMIX_process_create_with_arguments(args->name, args->code, 
						args->data, args->host,
						args->argc,args->argv);
				if(args->kill_time > SIMIX_get_clock()) {
					surf_timer_resource->extension_public->set(args->kill_time, 
							(void*) &SIMIX_process_kill,
							(void*) process);
				}
				xbt_free(args);
			}
			if(fun==SIMIX_process_kill) {
				process = arg;
				DEBUG2("Killing %s on %s", process->name, 
						process->simdata->host->name);
				SIMIX_process_kill(process);
			}
		}

		/* Wake up all process waiting for the action finish */
		xbt_dynar_foreach(resource_list, i, resource) {
			while ((action = xbt_swag_extract(resource->common_public->states.failed_action_set))) {
				smx_action = action->data;
				if (smx_action) {
					xbt_fifo_unshift(actions_failed,smx_action);
				}
			}
			while ((action =xbt_swag_extract(resource->common_public->states.done_action_set))) {
				smx_action = action->data;
				if (smx_action) {
					xbt_fifo_unshift(actions_done,smx_action);
				}
			}
		}
	}

	if (elapsed_time == -1) {
		if (xbt_swag_size(simix_global->process_list) == 0) {
			INFO0("Congratulations ! Simulation terminated : all processes are over");
		} else {
			INFO0("Oops ! Deadlock or code not perfectly clean.");
			__SIMIX_display_process_status();
			if(XBT_LOG_ISENABLED(simix, xbt_log_priority_debug) ||
					XBT_LOG_ISENABLED(simix_kernel, xbt_log_priority_debug)) {
				DEBUG0("Aborting!");
				xbt_abort();
			}
			INFO0("Return a Warning.");
		}
	}
	return elapsed_time;
}


void SIMIX_timer_set (double date, void *function, void *arg)
{
	surf_timer_resource->extension_public->set(date, function, arg);
}

int SIMIX_timer_get(void **function, void **arg)
{
	return surf_timer_resource->extension_public->get(function, arg);
}
