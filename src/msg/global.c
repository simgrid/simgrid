/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(global, msg,
				"Logging specific to MSG (global)");

MSG_Global_t msg_global = NULL;

/* static void MarkAsFailed(m_task_t t, TBX_HashTable_t failedProcessList); */
/* static xbt_fifo_t MSG_buildFailedHostList(long double a, long double b); */

/********************************* MSG **************************************/

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init(void)
{
  int argc=0;
  char **argv=NULL;

  CRITICAL0("Please stop using this function. Use MSG_global_init_args instead.");
  DIE_IMPOSSIBLE;
  MSG_global_init_args(&argc,argv);
}

void MSG_global_init_args(int *argc, char **argv)
{
  if (!msg_global) {
    msg_global = xbt_new0(s_MSG_Global_t,1);

    surf_init(argc, argv);	/* Initialize some common structures */
    xbt_context_init();
    msg_global->host = xbt_fifo_new();
    msg_global->process_to_run = xbt_fifo_new();
    msg_global->process_list = xbt_fifo_new();
    msg_global->max_channel = 0;
    msg_global->current_process = NULL;
    msg_global->registered_functions = xbt_dict_new();
  }
}

/** \ingroup msg_simulation
 * \brief Defines the verbosity of the simulation.
 */
void MSG_set_verbosity(MSG_outputmode_t mode)
{
  CRITICAL0("MSG_set_verbosity : Not implemented yet.");
}

/** \ingroup m_channel_management
 * \brief Set the number of channel in the simulation.
 *
 * This function has to be called to fix the number of channel in the
   simulation before creating any host. Indeed, each channel is
   represented by a different mailbox on each #m_host_t. This
   function can then be called only once. This function takes only one
   parameter.
 * \param number the number of channel in the simulation. It has to be >0
 */
MSG_error_t MSG_set_channel_number(int number)
{
  xbt_assert0((msg_global) && (msg_global->max_channel == 0), "Channel number already set!");

  msg_global->max_channel = number;

  return MSG_OK;
}

/** \ingroup m_simulation
 * \brief Set the sharing policy used for the links
 *
 * This function can be called to change the sharing policy used for the links 
   (see \ref paper_tcp). By default the store and forward mechanism is used
   with a parameter equal to 0.1. This function has to be called before creating 
   any link. 
 * \param mode the sharing policy used for the links: can be either 
   MSG_STORE_AND_FORWARD or MSG_TCP.
 * \param param a parameter for the sharing policy. It has to be >0. It is 
   currently used only for the MSG_STORE_AND_FORWARD flavor and represents the
   granularity of the communications (i.e. the packet size).
 */
MSG_error_t MSG_set_sharing_policy(MSG_sharing_t mode, long double param)
{
  CRITICAL0("MSG_set_sharing_policy: this function is now deprecated and useless. Store and forward does not exist anymore. Please stop using it.");
  
  return MSG_OK;
}

/** \ingroup m_channel_management
 * \brief Return the number of channel in the simulation.
 *
 * This function has to be called once the number of channel is fixed. I can't 
   figure out a reason why anyone would like to call this function but nevermind.
 * \return the number of channel in the simulation.
 */
int MSG_get_channel_number(void)
{
  xbt_assert0((msg_global)&&(msg_global->max_channel != 0), "Channel number not set yet!");

  return msg_global->max_channel;
}

/** \ingroup msg_simulation
 * \brief Launch the MSG simulation
 */
MSG_error_t MSG_main(void)
{
  m_process_t process = NULL;
  int nbprocess,i;
  long double Before=0.0;
  long double Now=0.0;
  double elapsed_time = 0.0;

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  surf_solve(); /* Takes traces into account. Returns 0.0 */
/* xbt_fifo_size(msg_global->process_to_run) */
  while (1) {
    xbt_context_empty_trash();
    while ((process = xbt_fifo_pop(msg_global->process_to_run))) {
/*       fprintf(stderr,"-> %s (%d)\n",process->name, process->simdata->PID); */
      msg_global->current_process = process;
      xbt_context_schedule(process->simdata->context);
      msg_global->current_process = NULL;
    }
    Before = MSG_getClock();
    elapsed_time = surf_solve();
    Now = MSG_getClock();

/*     fprintf(stderr, "====== %Lg =====\n",Now); */
/*     if (elapsed_time==0.0) { */
/*       fprintf(stderr, "No change in time\n"); */
/*     } */
    if (elapsed_time<0.0) {
/*       fprintf(stderr, "We're done %lg\n",elapsed_time); */
      break;
    }

    {
      surf_action_t action = NULL;
      surf_resource_t resource = NULL;
      m_task_t task = NULL;
      
      xbt_dynar_foreach(resource_list, i, resource) {
	while ((action =
	       xbt_swag_extract(resource->common_public->states.
				failed_action_set))) {
	  task = action->data;
	  if(task) {
	    int _cursor;
	    DEBUG1("** %s failed **",task->name);
/* 	    fprintf(stderr,"** %s **\n",task->name); */
	    xbt_dynar_foreach(task->simdata->sleeping,_cursor,process) {
	      DEBUG3("\t preparing to wake up %s(%d) on %s",	     
		     process->name,process->simdata->PID,
		     process->simdata->host->name);
	      xbt_fifo_unshift(msg_global->process_to_run, process);
	    }
	    process=NULL;
	  }
	}
	while ((action =
	       xbt_swag_extract(resource->common_public->states.
				done_action_set))) {
	  task = action->data;
	  if(task) {
	    int _cursor;
	    DEBUG1("** %s done **",task->name);
/* 	    fprintf(stderr,"** %s **\n",task->name); */
	    xbt_dynar_foreach(task->simdata->sleeping,_cursor,process) {
	      DEBUG3("\t preparing to wake up %s(%d) on %s",	     
		     process->name,process->simdata->PID,
		     process->simdata->host->name);
	      xbt_fifo_unshift(msg_global->process_to_run, process);
	    }
	    process=NULL;
	  }
	}
      }
    }
  }

  if ((nbprocess=xbt_fifo_size(msg_global->process_list)) == 0) {
    fprintf(stderr,
	    "MSG: Congratulations ! Simulation terminated : all process are over\n");
    return MSG_OK;
  } else {
    fprintf(stderr,"MSG: Oops ! Deadlock or code not perfectly clean.\n");
    fprintf(stderr,"MSG: %d processes are still running, waiting for something.\n",
	    nbprocess);
    /*  List the process and their state */
    fprintf(stderr,"MSG: <process>(<pid>) on <host>: <status>.\n");
    while ((process=xbt_fifo_pop(msg_global->process_list))) {
      simdata_process_t p_simdata = (simdata_process_t) process->simdata;
      simdata_host_t h_simdata=(simdata_host_t)p_simdata->host->simdata;
      

      fprintf(stderr,"MSG:  %s(%d) on %s: ",
	     process->name,p_simdata->PID,
	     p_simdata->host->name);

      if (process->simdata->blocked) 	  
	fprintf(stderr,"[blocked] ");
      if (process->simdata->suspended) 	  
	fprintf(stderr,"[suspended] ");

      for (i=0; i<msg_global->max_channel; i++) {
	if (h_simdata->sleeping[i] == process) {
	  fprintf(stderr,"Listening on channel %d.\n",i);
	  break;
	}
      }
      if (i==msg_global->max_channel) {
	if(p_simdata->waiting_task) {
	  if(p_simdata->waiting_task->simdata->compute) {
	    if(p_simdata->put_host) 
	      fprintf(stderr,"Trying to send a task on Host %s, channel %d.\n",
		      p_simdata->put_host->name, p_simdata->put_channel);
	    else 
	      fprintf(stderr,"Waiting for %s to finish.\n",p_simdata->waiting_task->name);
	  } else if (p_simdata->waiting_task->simdata->comm)
	    fprintf(stderr,"Waiting for %s to be finished transfered.\n",
		    p_simdata->waiting_task->name);
	  else
	    fprintf(stderr,"UNKNOWN STATUS. Please report this bug.\n");
	}
	else { /* Must be trying to put a task somewhere */
	  fprintf(stderr,"UNKNOWN STATUS. Please report this bug.\n");
	}
      } 
    }
    if(XBT_LOG_ISENABLED(msg, xbt_log_priority_debug) ||
       XBT_LOG_ISENABLED(global, xbt_log_priority_debug)) {
      DEBUG0("Aborting!");
      xbt_abort();
    }

    return MSG_WARNING;
  }
}

/* static void MarkAsFailed(m_task_t t, TBX_HashTable_t failedProcessList)  */
/* { */
/*   simdata_task_t simdata = NULL; */
/*   xbt_fifo_item_t i = NULL; */
/*   m_process_t p = NULL; */
  
/*   xbt_assert0((t!=NULL),"Invalid task"); */
/*   simdata = t->simdata; */

/* #define KILL(task) if(task) SG_failTask(task) */
/*   KILL(simdata->compute); */
/*   KILL(simdata->TCP_comm); */
/*   KILL(simdata->s[0]); */
/*   KILL(simdata->s[1]); */
/*   KILL(simdata->s[2]); */
/*   KILL(simdata->s[3]); */
/*   KILL(simdata->sleep); */
/* #undef KILL   */
/* /\*   if(simdata->comm) SG_failEndToEndTransfer(simdata->comm); *\/ */
  
/*   xbt_fifo_foreach(simdata->sleeping,i,p,m_process_t) { */
/*     if(!TBX_HashTable_isInList(failedProcessList,p,TBX_basicHash))  */
/*       TBX_HashTable_insert(failedProcessList,p,TBX_basicHash); */
/*   } */
  
/* } */

/* static xbt_fifo_t MSG_buildFailedHostList(long double begin, long double end) */
/* { */
/*   xbt_fifo_t failedHostList = xbt_fifo_new(); */
/*   m_host_t host = NULL; */
/*   xbt_fifo_item_t i; */

/*   xbt_fifo_foreach(msg_global->host,i,host,m_host_t) { */
/*     SG_Resource r= ((simdata_host_t) (host->simdata))->host; */

/*     if(SG_evaluateFailureTrace(r->failure_trace,begin,end)!=-1.0) */
/*       xbt_fifo_insert(failedHostList,host); */
/*   } */

/*   return failedHostList; */
/* } */

/** \ingroup msg_simulation
 * \brief Clean the MSG simulation
 */
MSG_error_t MSG_clean(void)
{
  xbt_fifo_item_t i = NULL;
  m_host_t h = NULL;
  m_process_t p = NULL;


  while((p=xbt_fifo_shift(msg_global->process_list))) {
    MSG_process_kill(p);
  }
  xbt_context_exit();

  xbt_fifo_foreach(msg_global->host,i,h,m_host_t) {
    __MSG_host_destroy(h);
  }
  xbt_fifo_free(msg_global->host);
  xbt_fifo_free(msg_global->process_to_run);
  xbt_fifo_free(msg_global->process_list);
  xbt_dict_free(&(msg_global->registered_functions));

  xbt_free(msg_global);
  surf_finalize();

  return MSG_OK;
}


/** \ingroup msg_easier_life
 * \brief A clock (in second).
 */
long double MSG_getClock(void) {
  return surf_get_clock();
}

