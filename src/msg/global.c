/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(global, msg,
				"Logging specific to MSG (global)");

int __stop_at_time = -1.0 ;

MSG_Global_t msg_global = NULL;

/* static void MarkAsFailed(m_task_t t, TBX_HashTable_t failedProcessList); */
/* static xbt_fifo_t MSG_buildFailedHostList(double a, double b); */

/** \defgroup msg_simulation   MSG simulation Functions
 *  \brief This section describes the functions you need to know to
 *  set up a simulation. You should have a look at \ref MSG_examples 
 *  to have an overview of their usage.
 */

/********************************* MSG **************************************/

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init(void)
{
  int argc=0;
  char **argv=NULL;

  CRITICAL0("Function MSG_global_init() is deprecated by MSG_global_init_args().");
  MSG_global_init_args(&argc,argv);
}

/** \ingroup msg_simulation
 * \brief Initialize some MSG internal data.
 */
void MSG_global_init_args(int *argc, char **argv)
{
  if (!msg_global) {
    surf_init(argc, argv);	/* Initialize some common structures. Warning, it sets msg_global=NULL */
     
    msg_global = xbt_new0(s_MSG_Global_t,1);

    xbt_context_init();
    msg_global->host = xbt_fifo_new();
    msg_global->process_to_run = xbt_fifo_new();
    msg_global->process_list = xbt_fifo_new();
    msg_global->max_channel = 0;
    msg_global->current_process = NULL;
    msg_global->registered_functions = xbt_dict_new();
    msg_global->PID = 1;
  }
}

/** \ingroup msg_easier_life
 * \brief Traces MSG events in the Paje format.
 */
void MSG_paje_output(const char *filename)
{
  int i;
  const char *paje_preembule="%EventDef	SetLimits	0\n"
    "%	StartTime	date\n"
    "%	EndTime	date\n"
    "%EndEventDef\n"
    "%EventDef	PajeDefineContainerType	1\n"
    "%	NewType	string\n"
    "%	ContainerType	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeDefineEventType	2\n"
    "%	NewType	string\n"
    "%	ContainerType	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeDefineStateType	3\n"
    "%	NewType	string\n"
    "%	ContainerType	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeDefineVariableType	4\n"
    "%	NewType	string\n"
    "%	ContainerType	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeDefineLinkType	5\n"
    "%	NewType	string\n"
    "%	ContainerType	string\n"
    "%	SourceContainerType	string\n"
    "%	DestContainerType	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeDefineEntityValue	6\n"
    "%	NewValue	string\n"
    "%	EntityType	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeCreateContainer	7\n"
    "%	Time	date\n"
    "%	NewContainer	string\n"
    "%	NewContainerType	string\n"
    "%	Container	string\n"
    "%	NewName	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeDestroyContainer	8\n"
    "%	Time	date\n"
    "%	Name	string\n"
    "%	Type	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeNewEvent	9\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeSetState	10\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeSetState	101\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%	FileName	string\n"
    "%	LineNumber	int\n"
    "%EndEventDef\n"
    "%EventDef	PajePushState	111\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%	FileName	string\n"
    "%	LineNumber	int\n"
    "%EndEventDef\n"
    "%EventDef	PajePushState	11\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%EndEventDef\n"
    "%EventDef	PajePopState	12\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeSetVariable	13\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	double\n"
    "%EndEventDef\n"
    "%EventDef	PajeAddVariable	14\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	double\n"
    "%EndEventDef\n"
    "%EventDef	PajeSubVariable	15\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	double\n"
    "%EndEventDef\n"
    "%EventDef	PajeStartLink	16\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%	SourceContainer	string\n"
    "%	Key	string\n"
    "%EndEventDef\n"
    "%EventDef	PajeEndLink	17\n"
    "%	Time	date\n"
    "%	EntityType	string\n"
    "%	Container	string\n"
    "%	Value	string\n"
    "%	DestContainer	string\n"
    "%	Key	string\n"
    "%EndEventDef\n";

  const char *type_definitions = "1	Sim_t	0	Simulation_t\n"
    "1	H_t	Sim_t	m_host_t\n"
    "1	P_t	H_t	m_process_t\n"
    "3	S_t	P_t	\"Process State\"\n"
    "6	E	S_t	Executing\n"
    "6	B	S_t	Blocked\n"
    "6	C	S_t	Communicating\n"
    "5	Comm	Sim_t	P_t	P_t	Communication_t\n";

  const char *ext = ".trace";
  int ext_len = strlen(ext);
  int len;
  m_host_t host;
  m_process_t process;
  xbt_fifo_item_t item = NULL;

  xbt_assert0(msg_global, "Initialize MSG first\n");
  xbt_assert0(!msg_global->paje_output, "Paje output already defined\n");
  xbt_assert0(filename, "Need a real file name\n");

  len = strlen(filename);
  if((len<ext_len) || (strncmp(filename+len-ext_len,ext,ext_len))) {
    CRITICAL2("%s does not end by \"%s\". It may cause troubles when using Paje\n",
	      filename,ext);
  }

  msg_global->paje_output=fopen(filename,"w");
  xbt_assert1(msg_global->paje_output, "Failed to open %s \n",filename);

  fprintf(msg_global->paje_output,"%s",paje_preembule);
  fprintf(msg_global->paje_output,"%s",type_definitions);

  /*    Channels    */
  for(i=0; i<msg_global->max_channel; i++) {
    fprintf(msg_global->paje_output, "6	COMM_%d	Comm	\"Channel %d\"\n" ,i,i);
  }
  fprintf(msg_global->paje_output,
	  "7	0.0	CUR	Sim_t	0	\"MSG simulation\"\n");

  /*    Hosts       */
  xbt_fifo_foreach(msg_global->host,item,host,m_host_t) {
    PAJE_HOST_NEW(host);
  }

  /*    Process       */
  xbt_fifo_foreach(msg_global->process_list,item,process,m_process_t) {
    PAJE_PROCESS_NEW(process);
  }
}

/** \ingroup msg_simulation
 * \brief Defines the verbosity of the simulation.
 */
void MSG_set_verbosity(MSG_outputmode_t mode)
{
  CRITICAL0("MSG_set_verbosity : Deprecated function. Use the XBT logging interface.");
}

/** \defgroup m_channel_management    Understanding channels
 *  \brief This section briefly describes the channel notion of MSG
 *  (#m_channel_t).
 *
 *  For convenience, the simulator provides the notion of channel
 *  that is close to the tag notion in MPI. A channel is not a
 *  socket. It doesn't need to be opened neither closed. It rather
 *  corresponds to the ports opened on the different machines.
 */


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
MSG_error_t MSG_set_sharing_policy(MSG_sharing_t mode, double param)
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
  double elapsed_time = 0.0;

  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  surf_solve(); /* Takes traces into account. Returns 0.0 */
/* xbt_fifo_size(msg_global->process_to_run) */
  while (1) {
    xbt_context_empty_trash();
    if(xbt_fifo_size(msg_global->process_to_run) && (elapsed_time>0)) {
      DEBUG0("**************************************************");
    }
    if((__stop_at_time>0) && (MSG_getClock() >= __stop_at_time)) {
      DEBUG0("Let's stop here!");
    }

    while ((process = xbt_fifo_pop(msg_global->process_to_run))) {
      DEBUG3("Scheduling  %s(%d) on %s",	     
	     process->name,process->simdata->PID,
	     process->simdata->host->name);
      msg_global->current_process = process;
      fflush(NULL);
      xbt_context_schedule(process->simdata->context);
      msg_global->current_process = NULL;
    }
    DEBUG1("%lg : Calling surf_solve",MSG_getClock());
    elapsed_time = surf_solve();
    DEBUG1("Elapsed_time %lg",elapsed_time);

/*     fprintf(stderr, "====== %lg =====\n",Now); */
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

      void *fun = NULL;
      void *arg = NULL;
      while (surf_timer_resource->extension_public->get(&fun,(void*)&arg)) {
	DEBUG2("got %p %p", fun, arg);
	if(fun==MSG_process_create_with_arguments) {
	  process_arg_t args = arg;
	  DEBUG2("Launching %s on %s", args->name, args->host->name);
	  process = MSG_process_create_with_arguments(args->name, args->code, 
						      args->data, args->host,
						      args->argc,args->argv);
	  if(args->kill_time > MSG_getClock()) {
	    surf_timer_resource->extension_public->set(args->kill_time, 
						       (void*) &MSG_process_kill,
						       (void*) process);
	  }
	  xbt_free(args);
	}
	if(fun==MSG_process_kill) {
	  process = arg;
	  DEBUG3("Killing %s(%d) on %s", process->name, process->simdata->PID, 
		 process->simdata->host->name);
	  MSG_process_kill(process);
	}
      }
      
      xbt_dynar_foreach(resource_list, i, resource) {
	while ((action =
	       xbt_swag_extract(resource->common_public->states.
				failed_action_set))) {
	  task = action->data;
	  if(task) {
	    int _cursor;
	    DEBUG1("** %s failed **",task->name);
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
    xbt_fifo_item_t item = NULL;
    fprintf(stderr,"MSG: Oops ! Deadlock or code not perfectly clean.\n");
    fprintf(stderr,"MSG: %d processes are still running, waiting for something.\n",
	    nbprocess);
    /*  List the process and their state */
    fprintf(stderr,"MSG: <process>(<pid>) on <host>: <status>.\n");
    xbt_fifo_foreach(msg_global->process_list,item,process,m_process_t) {
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

/* static xbt_fifo_t MSG_buildFailedHostList(double begin, double end) */
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
 * \brief Kill all running process

 * \param reset_PIDs should we reset the PID numbers. A negative
 *   number means no reset and a positive number will be used to set the PID
 *   of the next newly created process.
 */
int MSG_process_killall(int reset_PIDs)
{
  xbt_fifo_item_t i = NULL;
  m_process_t p = NULL;
  m_process_t self = MSG_process_self();

  while((p=xbt_fifo_shift(msg_global->process_list))) {
    if(p!=self) MSG_process_kill(p);
  }

  if(reset_PIDs>0) {
    msg_global->PID = reset_PIDs; 
    msg_global->session++;
 }

  xbt_context_empty_trash();

  if(self) {
    xbt_context_yield();
  }

  return msg_global->PID;
}

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

  if(msg_global->paje_output) {
    fclose(msg_global->paje_output);
    msg_global->paje_output = NULL;
  }
  free(msg_global);
  surf_finalize();

  return MSG_OK;
}


/** \ingroup msg_easier_life
 * \brief A clock (in second).
 * \deprecated Use MSG_get_clock
 */
double MSG_getClock(void) {
  return surf_get_clock();
}

/** \ingroup msg_easier_life
 * \brief A clock (in second).
 */
double MSG_get_clock(void) {
  return surf_get_clock();
}

