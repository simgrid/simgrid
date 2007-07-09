/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the implementation of the wrapper functions used to interface
 * the java object with the native functions of the MSG API.
 */
#include "msg/msg.h"
#include "msg/private.h"
#include "java/jxbt_context.h"

#include "jmsg_process.h"
#include "jmsg_host.h"
#include "jmsg_task.h"
#include "jmsg_parallel_task.h"
#include "jmsg_channel.h"
#include "jxbt_utilities.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(jmsg,"MSG for Java(TM)");

/* header for windows */
#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "jmsg.h"


#ifdef WIN32
  static DWORD __current_thread_id = 0;

  int is_main_thread() {
    return (GetCurrentThreadId() == __current_thread_id);
  }

#else /* !WIN32 */

  static pthread_t __current_thread_id = 0;

  int is_main_thread() {
    return (pthread_self() == __current_thread_id);
  }
#endif


/*
 * The MSG process connected functions implementation.                                 
 */

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processCreate(JNIEnv* env, jclass cls, jobject jprocess_arg, jobject jhost) {
  jobject jprocess;		/* the global reference to the java process instance	*/
  jstring jname;		/* the name of the java process instance		*/
  const char* name;		/* the C name of the process	       			*/
  m_process_t process;		/* the native process to create				*/


  DEBUG4("Java_simgrid_msg_Msg_processCreate(env=%p,cls=%p,jproc=%p,jhost=%p)",
	 env,cls,jprocess_arg,jhost);
  /* get the name of the java process */
  jname = jprocess_get_name(jprocess_arg,env);

  if (!jname) {
    jxbt_throw_null(env,xbt_strdup("Internal error: Process name cannot be NULL"));
    return;
  }
	
  /* allocate the data of the simulation */
  process = xbt_new0(s_m_process_t,1);
  process->simdata = xbt_new0(s_simdata_process_t,1);
    
  /* create a global java process instance */
  jprocess = jprocess_new_global_ref(jprocess_arg,env);

  if(!jprocess) {
    free(process->simdata);
    free(process);
    jxbt_throw_jni(env, "Can't get a global ref to the java process");
    return;
  }
	
  /* bind the java process instance to the native host */
  jprocess_bind(jprocess,process,env);
	
  /* build the C name of the process */
  name = (*env)->GetStringUTFChars(env, jname, 0);
  process->name = xbt_strdup(name);
  (*env)->ReleaseStringUTFChars(env, jname, name);
	
  process->simdata->m_host = jhost_get_native(env,jhost);
  if( ! (process->simdata->m_host) ) { /* not binded */
    free(process->simdata);
    free(process->data);
    free(process);
    jxbt_throw_notbound(env,"host",jhost);
    return;
  }
  process->simdata->PID = msg_global->PID++;
    
  /* create a new context */
  DEBUG8("fill in process %s/%s (pid=%d) %p (sd=%p, host=%p, host->sd=%p); env=%p",
	 process->name,process->simdata->m_host->name,
	 process->simdata->PID,process,
	 process->simdata, process->simdata->m_host, process->simdata->m_host->simdata,
	 env);
  
  SIMIX_jprocess_create(process->name,
			process->simdata->m_host->simdata->s_host, 
			/*data*/ (void*)process,
			jprocess_arg,env,
			__MSG_process_cleanup,
			&process->simdata->s_process);
  DEBUG1("context created (s_process=%p)",process->simdata->s_process);


  if (SIMIX_process_self()) { /* someone created me */
    process->simdata->PPID = MSG_process_get_PID(SIMIX_process_self()->data);
  } else {
    process->simdata->PPID = -1;
  }
    
  process->simdata->last_errno = MSG_OK;
    

#ifdef KILLME    
  /* add the process in the list of the process of the host */
  xbt_fifo_unshift(host->simdata->process_list, process);
    
  self = msg_global->current_process;
    
  process->simdata->context->env = env;
    
  /* start the java process */
  xbt_context_start(process->simdata->context); 
	
  msg_global->current_process = self;
#endif
    
  /* add the process to the list of the processes of the simulation */
  xbt_fifo_unshift(msg_global->process_list, process);
  	
  /* add the process to the list of the processes to run in the simulation */
  //  xbt_fifo_unshift(msg_global->process_to_run, process);
    
  //  PAJE_PROCESS_NEW(process);
  //#endif
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processSuspend(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if(!process) { 
    jxbt_throw_notbound(env,"process",jprocess);
    return;
  }
	
  /* try to suspend the process */
  if(MSG_OK != MSG_process_suspend(process)) 
    jxbt_throw_native(env, xbt_strdup("MSG_process_suspend() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processResume(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if(!process) { 
    jxbt_throw_notbound(env,"process",jprocess);
    return;
  }
	
  /* try to resume the process */
  if(MSG_OK != MSG_process_resume(process))
    jxbt_throw_native(env, xbt_strdup("MSG_process_resume() failed"));
}

JNIEXPORT jboolean JNICALL 
Java_simgrid_msg_Msg_processIsSuspended(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if(!process) { 
    jxbt_throw_notbound(env,"process",jprocess);
    return 0;
  }

  /* true is the process is suspended, false otherwise */
  return (jboolean)MSG_process_is_suspended(process);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processKill(JNIEnv* env, jclass cls, jobject jprocess) {
  /* get the native instances from the java ones */
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if(!process) { 
    jxbt_throw_notbound(env,"process",jprocess);
    return;
  }

  /* delete the global reference */
  jprocess_delete_global_ref(SIMIX_process_get_jprocess(process->simdata->s_process),env);
	
  /* kill the native process (this wrapper is call by the destructor of the java 
   * process instance)
   */
  MSG_process_kill(process);
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_processGetHost(JNIEnv* env, jclass cls, jobject jprocess) {
  /* get the native instances from the java ones */
  m_process_t process = jprocess_to_native_process(jprocess,env);
  m_host_t host;
	
  if(!process) { 
    jxbt_throw_notbound(env,"process",jprocess);
    return NULL;
  }

  host = MSG_process_get_host(process);
	
  if(!host->data) {
    jxbt_throw_native(env, xbt_strdup("MSG_process_get_host() failed"));
    return NULL;
  }

  /* return the global reference to the java host instance */
  return (jobject)host->data;
	
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_processFromPID(JNIEnv* env, jclass cls, jint PID) {
  m_process_t process = MSG_process_from_PID(PID);

  if(!process) {
    jxbt_throw_native(env, bprintf("MSG_process_from_PID(%d) failed",PID));
    return NULL;
  }

  if(!SIMIX_process_get_jprocess(process->simdata->s_process)) {
    jxbt_throw_native(env, xbt_strdup("SIMIX_process_get_jprocess() failed"));
    return NULL;
  }

  return (jobject)SIMIX_process_get_jprocess(process->simdata->s_process);
}


JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_processGetPID(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if(!process) { 
    jxbt_throw_notbound(env,"process",jprocess);
    return 0;
  }

  return (jint)MSG_process_get_PID(process);	
}


JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_processGetPPID(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if(!process) {
    jxbt_throw_notbound(env,"process",jprocess);
    return 0;
  }

  return (jint)MSG_process_get_PPID(process);
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_processSelf(JNIEnv* env, jclass cls) {
  m_process_t process = MSG_process_self();
  jobject jprocess;

  if(!process) {
    jxbt_throw_native(env, xbt_strdup("MSG_process_self() failed"));
    return NULL;
  }

  jprocess = SIMIX_process_get_jprocess(process->simdata->s_process);

  if(!jprocess)
    jxbt_throw_native(env, xbt_strdup("SIMIX_process_get_jprocess() failed"));
  
  return jprocess;
}


JNIEXPORT jint JNICALL
Java_simgrid_msg_Msg_processSelfPID(JNIEnv* env, jclass cls) {
  return (jint)MSG_process_self_PID();
}


JNIEXPORT jint JNICALL
Java_simgrid_msg_Msg_processSelfPPID(JNIEnv* env, jclass cls) {
  return (jint)MSG_process_self_PPID();
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processChangeHost(JNIEnv* env, jclass cls, jobject jprocess, jobject jhost){
  m_host_t host = jhost_get_native(env,jhost);
  m_process_t process = jprocess_to_native_process(jprocess,env);
	
  if(!process) {
    jxbt_throw_notbound(env,"process",jprocess);
    return;
  }
	
  if(!host) {
    jxbt_throw_notbound(env,"host",jhost);
    return;
  }

  /* try to change the host of the process */
  if(MSG_OK != MSG_process_change_host(process,host))
    jxbt_throw_native(env, xbt_strdup("MSG_process_change_host() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processWaitFor(JNIEnv* env, jclass cls,jdouble seconds) {
  if(MSG_OK != MSG_process_sleep((double)seconds))
    jxbt_throw_native(env, 
		       bprintf("MSG_process_change_host(%f) failed", (double)seconds));
}


/***************************************************************************************
 * The MSG host connected functions implementation.                                    *
 ***************************************************************************************/

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_hostGetByName(JNIEnv* env, jclass cls, jstring jname) {
  m_host_t host;		/* native host						*/
  jobject jhost;		/* global reference to the java host instance returned	*/
	
  /* get the C string from the java string*/
  const char* name = (*env)->GetStringUTFChars(env, jname, 0);
	
  /* get the host by name	(the hosts are created during the grid resolution) */
  host = MSG_get_host_by_name(name);
  DEBUG2("MSG gave %p as native host (simdata=%p)",host,host->simdata);
	
  (*env)->ReleaseStringUTFChars(env, jname, name); 

  if(!host) {/* invalid name */
    jxbt_throw_host_not_found(env,name);
    return NULL;
  }

  if(!host->data) { /* native host not associated yet with java host */
		
    /* instanciate a new java host */
    jhost = jhost_new_instance(env);

    if(!jhost) {
      jxbt_throw_jni(env,"java host instantiation failed");
      return NULL;
    }
		
    /* get a global reference to the newly created host */
    jhost = jhost_ref(env,jhost);

    if(!jhost) {
      jxbt_throw_jni(env,"new global ref allocation failed");
      return NULL;
    }
		
    /* bind the java host and the native host */
    jhost_bind(jhost,host,env);
		
    /* the native host data field is set with the global reference to the 
     * java host returned by this function 
     */
    host->data = (void*)jhost;
  }

  /* return the global reference to the java host instance */
  return (jobject)host->data;			
}

JNIEXPORT jstring JNICALL 
Java_simgrid_msg_Msg_hostGetName(JNIEnv* env, jclass cls, jobject jhost) {
  m_host_t host = jhost_get_native(env,jhost);

  if(!host) {
    jxbt_throw_notbound(env,"host",jhost);
    return NULL;
  }

  return (*env)->NewStringUTF(env,host->name);
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_hostGetNumber(JNIEnv* env, jclass cls) {
  return (jint)MSG_get_host_number();
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_Msg_hostSelf(JNIEnv* env, jclass cls) {
  jobject jhost;
	
  m_host_t host = MSG_host_self();
	
  if(!host->data) {
    /* the native host not yet associated with the java host instance */
    
    /* instanciate a new java host instance */
    jhost = jhost_new_instance(env);
    
    if(!jhost) {
      jxbt_throw_jni(env,"java host instantiation failed");
      return NULL;
    }
    
    /* get a global reference to the newly created host */
    jhost = jhost_ref(env,jhost);
    
    if(!jhost) {
      jxbt_throw_jni(env,"global ref allocation failed");
      return NULL;
    }

    /* Bind & store it */
    jhost_bind(jhost,host,env);
    host->data = (void*)jhost;
  } else {
    jhost = (jobject)host->data;
  }
	
  return jhost;
}

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_hostGetSpeed(JNIEnv* env, jclass cls, jobject jhost) {
  m_host_t host = jhost_get_native(env,jhost);

  if(!host) {
    jxbt_throw_notbound(env,"host",jhost);
    return -1;
  }
  
  return (jdouble)MSG_get_host_speed(host);	
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_hostGetLoad(JNIEnv* env, jclass cls, jobject jhost) {
  m_host_t host = jhost_get_native(env,jhost);

  if(!host) {
    jxbt_throw_notbound(env,"host",jhost);
    return -1;
  }

  return (jint)MSG_get_host_msgload(host);	
}


JNIEXPORT jboolean JNICALL 
Java_simgrid_msg_Msg_hostIsAvail(JNIEnv* env, jclass cls, jobject jhost) {
  m_host_t host = jhost_get_native(env,jhost);
  
  if(!host) {
    jxbt_throw_notbound(env,"host",jhost);
    return 0;
  }

  return (jboolean)MSG_host_is_avail(host);
}


/***************************************************************************************
 * The MSG task connected functions implementation.                                    *
 ***************************************************************************************/

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_taskCreate(JNIEnv* env, jclass cls, jobject jtask, jstring jname, 
				jdouble jcomputeDuration, jdouble jmessageSize) {
  m_task_t task;	/* the native task to create				*/
  const char* name;	/* the name of the task					*/
	
  if(jcomputeDuration < 0) {
    jxbt_throw_illegal(env,bprintf("Task ComputeDuration (%f) cannot be negative",
				    (double)jcomputeDuration));
    return;
  }

  if(jmessageSize < 0) {
    jxbt_throw_illegal(env,bprintf("Task MessageSize (%f) cannot be negative",
				    (double)jmessageSize));
    return;
  }

  if(!jname) {
    jxbt_throw_null(env,xbt_strdup("Task name cannot be null"));
    return;
  }
	
  /* get the C string from the java string*/
  name = (*env)->GetStringUTFChars(env, jname, 0);
	
  /* create the task */
  task = MSG_task_create(name,(double)jcomputeDuration,(double)jmessageSize,NULL);
	
  (*env)->ReleaseStringUTFChars(env, jname, name); 
	
  /* bind & store the task */
  jtask_bind(jtask,task,env);

  /* allocate a new global reference to the java task instance */	
  task->data = (void*)jtask_new_global_ref(jtask,env);

  if ( ! task->data )
    jxbt_throw_jni(env,"global ref allocation failed");
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallel_taskCreate(JNIEnv* env, jclass cls, jobject jtask_arg, jstring jname, 
					 jobjectArray jhosts,jdoubleArray jcomputeDurations_arg, jdoubleArray jmessageSizes_arg) {

  m_task_t task;	/* the native parallel task to create		*/
  const char* name;	/* the name of the task				*/
  jobject jtask;	/* the global reference to the java parallel task instance	*/
  int host_count;
  m_host_t* hosts;
  double* computeDurations;
  double* messageSizes;
  jdouble *jcomputeDurations;
  jdouble *jmessageSizes;

  jobject jhost;
  int index;


  if (!jcomputeDurations_arg) {
    jxbt_throw_null(env,xbt_strdup("Parallel task compute durations cannot be null"));
    return;
  }

  if(!jmessageSizes_arg){
    jxbt_throw_null(env,xbt_strdup("Parallel task message sizes cannot be null"));
    return;
  }

  if(!jname) {
    jxbt_throw_null(env,xbt_strdup("Parallel task name cannot be null"));
    return;
  }

  host_count = (int)(*env)->GetArrayLength(env,jhosts);
	

  hosts = (m_host_t*)calloc(host_count,sizeof(m_host_t));
  computeDurations = (double*)calloc(host_count,sizeof(double));
  messageSizes = (double*)calloc(host_count,sizeof(double));

  jcomputeDurations = (*env)->GetDoubleArrayElements(env,jcomputeDurations_arg, 0);
  jmessageSizes = (*env)->GetDoubleArrayElements(env,jmessageSizes_arg, 0);

  for(index = 0; index < host_count; index++) {
    jhost = (*env)->GetObjectArrayElement(env,jhosts,index);
    hosts[index] = jhost_get_native(env,jhost);
    computeDurations[index] = jcomputeDurations[index];
    messageSizes[index] = jmessageSizes[index];
  }

  (*env)->ReleaseDoubleArrayElements(env,jcomputeDurations_arg,jcomputeDurations,0);
  (*env)->ReleaseDoubleArrayElements(env,jmessageSizes_arg,jmessageSizes,0);

	
  /* get the C string from the java string*/
  name = (*env)->GetStringUTFChars(env, jname, 0);
	
  task = MSG_parallel_task_create(name,host_count,hosts,computeDurations,messageSizes,NULL);

  (*env)->ReleaseStringUTFChars(env, jname, name); 
	
  /* allocate a new global reference to the java task instance */
  jtask = jparallel_task_ref(env,jtask_arg);
	
  /* associate the java task object and the native task */
  jparallel_task_bind(jtask,task,env);

  task->data = (void*)jtask;

  if(!(task->data))
    jxbt_throw_jni(env,"global ref allocation failed");
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_taskGetSender(JNIEnv* env , jclass cls , jobject jtask) {
  m_process_t process;
	
  m_task_t task = jtask_to_native_task(jtask,env);
	
  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return NULL;
  }
	
  process = MSG_task_get_sender(task);
  return SIMIX_process_get_jprocess(process->simdata->s_process);
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetSender(JNIEnv* env , jclass cls , jobject jtask) {
  m_task_t task;
  m_process_t process;
	
  task = jparallel_task_to_native_parallel_task(jtask,env);

  if(!task) {
    jxbt_throw_notbound(env,"task",jtask);
    return NULL;
  }
	
  process = MSG_task_get_sender(task);
	
  return SIMIX_process_get_jprocess(process->simdata->s_process);
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_taskGetSource(JNIEnv* env , jclass cls, jobject jtask) {
  m_host_t host;
  m_task_t task = jtask_to_native_task(jtask,env);

  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return NULL;
  }
	
  host = MSG_task_get_source(task);

  if(! host->data) {
    jxbt_throw_native(env, xbt_strdup("MSG_task_get_source() failed"));
    return NULL;
  }
	
  return (jobject)host->data;	
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetSource(JNIEnv* env , jclass cls, jobject jtask) {
  m_task_t task = jparallel_task_to_native_parallel_task(jtask,env);
  m_host_t host;

  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return NULL;
  }
	
  host = MSG_task_get_source(task);
  
  if(! host->data ) {
    jxbt_throw_native(env, xbt_strdup("MSG_task_get_source() failed"));
    return NULL;
  }
	
  return (jobject)host->data;	
}


JNIEXPORT jstring JNICALL 
Java_simgrid_msg_Msg_taskGetName(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t task = jtask_to_native_task(jtask,env);

  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return NULL;
  }

  return (*env)->NewStringUTF(env,task->name);		
}

JNIEXPORT jstring JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetName(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return NULL;
  }
	
  return (*env)->NewStringUTF(env,ptask->name);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_taskCancel(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jtask_to_native_task(jtask,env);
 
  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
	
  if(MSG_OK != MSG_task_cancel(ptask))
    jxbt_throw_native(env, xbt_strdup("MSG_task_cancel() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskCancel(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
	
  if(MSG_OK != MSG_task_cancel(ptask))
    jxbt_throw_native(env, xbt_strdup("MSG_task_cancel() failed"));
}


JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_taskGetComputeDuration(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jtask_to_native_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return -1;
  }
  return (jdouble)MSG_task_get_compute_duration(ptask);
}

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetComputeDuration(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return -1;
  }
  return (jdouble)MSG_task_get_compute_duration(ptask);
}

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_taskGetRemainingDuration(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jtask_to_native_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return -1;
  }
  return (jdouble)MSG_task_get_remaining_computation(ptask);
}

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_paralleTaskGetRemainingDuration(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return -1;
  }
  return (jdouble)MSG_task_get_remaining_computation(ptask);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_taskSetPriority(JNIEnv* env, jclass cls, jobject jtask, jdouble priority) {
  m_task_t task = jtask_to_native_task(jtask,env);

  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
  MSG_task_set_priority(task,(double)priority);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskSetPriority(JNIEnv* env, jclass cls, 
					     jobject jtask, jdouble priority) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
  MSG_task_set_priority(ptask,(double)priority);
}


JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_taskDestroy(JNIEnv* env, jclass cls, jobject jtask_arg) {

  /* get the native task */
  m_task_t task = jtask_to_native_task(jtask_arg,env);
  jobject jtask;

  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
  jtask = (jobject)task->data;

  if(MSG_OK != MSG_task_destroy(task))
    jxbt_throw_native(env, xbt_strdup("MSG_task_destroy() failed"));

  /* delete the global reference to the java task object */
  jtask_delete_global_ref(jtask,env);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskDestroy(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
	
  /* unbind jobj & native one */
  jparallel_task_bind(jtask,0,env);

  /* delete the global reference to the java parallel task object */
  jparallel_task_unref(env,jtask);

  /* free allocated memory */
  if(ptask->simdata->comm_amount)
    free(ptask->simdata->comm_amount);

  if(ptask->simdata->host_list)
    free(ptask->simdata->host_list);

  if(MSG_OK != MSG_task_destroy(ptask))
    jxbt_throw_native(env, xbt_strdup("MSG_task_destroy() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_taskExecute(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t task = jtask_to_native_task(jtask,env);

  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }

  if(MSG_OK != MSG_task_execute(task))
    jxbt_throw_native(env, xbt_strdup("MSG_task_execute() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskExecute(JNIEnv* env, jclass cls, jobject jtask) {
  m_task_t ptask = jparallel_task_to_native_parallel_task(jtask,env);

  if(!ptask){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
	
  if(MSG_OK != MSG_parallel_task_execute(ptask))
    jxbt_throw_native(env, xbt_strdup("MSG_parallel_task_execute() failed"));
}

/***************************************************************************************
 * The MSG channel connected functions implementation.                                 *
 ***************************************************************************************/

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_channelGet(JNIEnv* env, jclass cls, jobject jchannel) {
  m_task_t task = NULL;
	
  if(MSG_OK != MSG_task_get(&task,(int)jchannel_get_id(jchannel,env))) {
    jxbt_throw_native(env, xbt_strdup("MSG_task_get() failed"));
    return NULL;
  }
	
  return (jobject)task->data;
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_channelGetWithTimeout(JNIEnv* env, jclass cls, 
					   jobject jchannel, jdouble jtimeout) {
  m_task_t task = NULL;
  int id = (int)jchannel_get_id(jchannel,env);
	
  if(MSG_OK != MSG_task_get_with_time_out(&task,id,(double)jtimeout)) {
    jxbt_throw_native(env, xbt_strdup("MSG_task_get_with_time_out() failed"));
    return NULL;
  }
	
  return (jobject)task->data;
}

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_channelGetFromHost(JNIEnv* env, jclass cls, 
					jobject jchannel, jobject jhost) {
  m_host_t host = jhost_get_native(env,jhost);
  m_task_t task = NULL;	
  int id = (int)jchannel_get_id(jchannel,env);

  if(!host){
    jxbt_throw_notbound(env,"host",jhost);
    return NULL;
  }  
  if(MSG_OK != MSG_task_get_from_host(&task,id,host)){
    jxbt_throw_native(env, xbt_strdup("MSG_task_get_from_host() failed"));
    return NULL;
  }
  if(!(task->data)){
    jxbt_throw_notbound(env,"task",task);
    return NULL;
  }

  return (jobject)task->data;
}

JNIEXPORT jboolean JNICALL 
Java_simgrid_msg_Msg_channelHasPendingCommunication(JNIEnv* env, jclass cls, jobject jchannel) {
  int id = (int)jchannel_get_id(jchannel,env);
	
  return (jboolean)MSG_task_Iprobe(id);
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_channelGetCommunicatingProcess(JNIEnv* env, jclass cls, jobject jchannel) {
  int id =jchannel_get_id(jchannel,env);
	
  return (jint)MSG_task_probe_from(id);
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_channelGetHostWaitingTasks(JNIEnv* env, jclass cls, 
						jobject jchannel, jobject jhost) {
  int id = (int)jchannel_get_id(jchannel,env);
  m_host_t host = jhost_get_native(env,jhost);

  if(!host){
    jxbt_throw_notbound(env,"host",jhost);
    return -1;
  }

  return (jint)MSG_task_probe_from_host(id,host);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_channelPut(JNIEnv* env, jclass cls, 
				jobject jchannel, jobject jtask, jobject jhost) {

  if(MSG_OK != MSG_task_put(jtask_to_native_task(jtask,env),
			    jhost_get_native(env,jhost),
			    (int)jchannel_get_id(jchannel,env)))
    jxbt_throw_native(env, xbt_strdup("MSG_task_put() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_channelPutWithTimeout(JNIEnv* env, jclass cls, 
					   jobject jchannel, jobject jtask, jobject jhost,
					   jdouble jtimeout) {
  m_task_t task = jtask_to_native_task(jtask,env);
  int id = (int)jchannel_get_id(jchannel,env);
  m_host_t host = jhost_get_native(env,jhost);

  if(!host){
    jxbt_throw_notbound(env,"host",jhost);
    return;
  }
  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
	
  if(MSG_OK != MSG_task_put_with_timeout(task,host,id,(double)jtimeout))
    jxbt_throw_native(env, xbt_strdup("MSG_task_put_with_timeout() failed"));
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_channelPutBounded(JNIEnv* env, jclass cls, 
				       jobject jchannel, jobject jtask, jobject jhost, 
				       jdouble jmaxRate) {
  m_task_t task = jtask_to_native_task(jtask,env);
  int id = (int)jchannel_get_id(jchannel,env);
  m_host_t host = jhost_get_native(env,jhost);

  if(!host){
    jxbt_throw_notbound(env,"host",jhost);
    return;
  }
  if(!task){
    jxbt_throw_notbound(env,"task",jtask);
    return;
  }
	 
  if(MSG_OK != MSG_task_put_bounded(task,host,id,(double)jmaxRate))
    jxbt_throw_native(env, xbt_strdup("MSG_task_put_bounded() failed"));
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_channelWait(JNIEnv* env, jclass cls, jobject jchannel, jdouble timeout) {
  int PID;
  int id = (int)jchannel_get_id(jchannel,env);
	
  if(MSG_OK != MSG_channel_select_from(id,(double)timeout,&PID)) {
    jxbt_throw_native(env, xbt_strdup("MSG_channel_select_from() failed"));
    return 0;
  }
	
  return (jint)PID;
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_channelGetNumber(JNIEnv* env, jclass cls) {
  return (jint)MSG_get_channel_number();
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_channelSetNumber(JNIEnv* env , jclass cls,jint channelNumber) {
  MSG_set_channel_number((int)channelNumber);
}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_getErrCode(JNIEnv* env, jclass cls) {
  return (jint)MSG_get_errno();
}

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_getClock(JNIEnv* env, jclass cls) {
  return (jdouble)MSG_get_clock();
}


JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_init(JNIEnv* env, jclass cls, jobjectArray jargs) {
	
  char** argv = NULL;
  int index;
  int argc = 0;
  jstring jval;
  const char* tmp;
	
  if(jargs)
    argc = (int)(*env)->GetArrayLength(env,jargs);

  argc++;
	
  argv = (char**)calloc(argc,sizeof(char*));
	 
  argv[0] = strdup("java");
	
  for(index = 0; index < argc -1; index++) {
    jval = (jstring)(*env)->GetObjectArrayElement(env,jargs,index);
    
    tmp = (*env)->GetStringUTFChars(env, jval, 0);
    
    argv[index +1] = strdup(tmp);

    (*env)->ReleaseStringUTFChars(env, jval, tmp); 
  }
	
  MSG_global_init(&argc,argv);

  for(index = 0; index < argc; index++)
    free(argv[index]);
	
  free(argv);

#ifdef WIN32
  __current_thread_id = GetCurrentThreadId();
#else
  __current_thread_id = pthread_self();
#endif
	
}

JNIEXPORT void JNICALL
JNICALL Java_simgrid_msg_Msg_run(JNIEnv* env, jclass cls) {
  xbt_fifo_item_t item = NULL;
  m_host_t host = NULL;
  jobject jhost;

  /* Run everything */
  if(MSG_OK != MSG_main())
    jxbt_throw_native(env, xbt_strdup("MSG_main() failed"));

  DEBUG0("MSG_main finished. Bail out before cleanup since there is a bug in this part.");
  SIMIX_display_process_status();   
  exit(0); /* FIXME */
   
  DEBUG0("Clean java world");
  /* Cleanup java hosts */
  xbt_fifo_foreach(msg_global->host,item,host,m_host_t) {
    jhost = (jobject)host->data;
	
    if(jhost)
      jhost_unref(env,jhost);	
  }
  DEBUG0("Clean native world");
  /* cleanup native stuff */
  if(MSG_OK != MSG_clean())
    jxbt_throw_native(env, xbt_strdup("MSG_main() failed"));

}

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_processKillAll(JNIEnv* env, jclass cls, jint jresetPID) {
  return (jint)MSG_process_killall((int)jresetPID);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_createEnvironment(JNIEnv* env, jclass cls,jstring jplatformFile) {
	
  const char* platformFile = (*env)->GetStringUTFChars(env, jplatformFile, 0);
	
  MSG_create_environment(platformFile);
	
  (*env)->ReleaseStringUTFChars(env, jplatformFile, platformFile); 
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_waitSignal(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t m_process = jprocess_to_native_process(jprocess,env);
  smx_process_t s_process;

  xbt_os_mutex_t ctx_mutex, creation_mutex;
  xbt_os_cond_t ctx_cond, creation_cond;

  DEBUG3("Msg_waitSignal(m_process=%p %s/%s)",
	 m_process,m_process->name,m_process->simdata->m_host->name);
  if (!m_process){
    jxbt_throw_notbound(env,"process",jprocess);
    return;
  }

  s_process = m_process->simdata->s_process;

  if (s_process == NULL) {
    jxbt_throw_notbound(env,"SIMIX process",jprocess);
    return;
  }

  ctx_mutex = SIMIX_process_get_jmutex(s_process);
  ctx_cond = SIMIX_process_get_jcond(s_process);

  creation_mutex = xbt_creation_mutex_get();
  creation_cond = xbt_creation_cond_get();

  xbt_os_mutex_lock(creation_mutex);
  xbt_os_mutex_lock(ctx_mutex);
  xbt_os_cond_signal( creation_cond );
  xbt_os_mutex_unlock( creation_mutex );
  xbt_os_cond_wait(ctx_cond, ctx_mutex);
  xbt_os_mutex_unlock(ctx_mutex);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processExit(JNIEnv* env, jclass cls, jobject jprocess) {
  m_process_t process = jprocess_to_native_process(jprocess,env);

  if (!process){
    jxbt_throw_notbound(env,"process",jprocess);
    return;
  }
  MSG_process_kill(process);
}

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_pajeOutput(JNIEnv* env, jclass cls, jstring jpajeFile) {
  const char*pajeFile = (*env)->GetStringUTFChars(env, jpajeFile, 0);
	
  MSG_paje_output(pajeFile);
	
  (*env)->ReleaseStringUTFChars(env, jpajeFile, pajeFile); 
}


JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_info(JNIEnv * env, jclass cls, jstring js) {
  const char* s = (*env)->GetStringUTFChars(env,js,0);
  INFO1("%s",s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}

JNIEXPORT jobjectArray JNICALL
Java_simgrid_msg_Msg_allHosts(JNIEnv * env, jclass cls_arg) {
  int index;
  jobjectArray jtable;
  jobject jhost;
  jstring jname;
  m_host_t host;

  int count = xbt_fifo_size(msg_global->host);
  m_host_t* table = (m_host_t *)xbt_fifo_to_array(msg_global->host);
	
  jclass cls = jxbt_get_class(env,"simgrid/msg/Host");
	
  if(!cls){
    return NULL;
  }

  jtable = (*env)->NewObjectArray(env,(jsize)count,cls,NULL);
	
  if(!jtable) {
    jxbt_throw_jni(env,"Hosts table allocation failed");
    return NULL;
  }
	
  for(index = 0; index < count; index++) {
    host = table[index];
    jhost = (jobject)(host->data);
    
    if(!jhost) {
      jname = (*env)->NewStringUTF(env,host->name);
      
      jhost = Java_simgrid_msg_Msg_hostGetByName(env,cls_arg,jname);
      /* FIXME: leak of jname ? */
    }
    
    (*env)->SetObjectArrayElement(env, jtable, index, jhost);
  }

  return jtable;
}

  
