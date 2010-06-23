/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"
#include "msg/private.h"
#include "simix/private.h"
#include "simix/smx_context_java.h"

#include "jmsg_process.h"
#include "jmsg_host.h"
#include "jmsg_task.h"
#include "jmsg_application_handler.h"
#include "jxbt_utilities.h"

#include "jmsg.h"
#include "msg/mailbox.h"
#include "surf/surfxml_parse.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static JavaVM *__java_vm = NULL;

static jobject native_to_java_process(m_process_t process);

JavaVM *get_java_VM(void)
{
  return __java_vm;
}

JNIEnv *get_current_thread_env(void)
{
  JNIEnv *env;

  (*__java_vm)->AttachCurrentThread(__java_vm, (void **) &env, NULL);

  return env;
}

static jobject native_to_java_process(m_process_t process)
{
  return ((smx_ctx_java_t)
          (process->simdata->s_process->context))->jprocess;
}

/*
 * The MSG process connected functions implementation.                                 
 */

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processCreate(JNIEnv * env, jclass cls,
                                         jobject jprocess_arg, jobject jhost)
{
  jobject jprocess;             /* the global reference to the java process instance    */
  jstring jname;                /* the name of the java process instance                */
  const char *name;             /* the C name of the process                            */
  m_process_t process;          /* the native process to create                         */
  char alias[MAX_ALIAS_NAME + 1] = { 0 };
  msg_mailbox_t mailbox;

  DEBUG4
    ("Java_simgrid_msg_MsgNative_processCreate(env=%p,cls=%p,jproc=%p,jhost=%p)",
     env, cls, jprocess_arg, jhost);
  /* get the name of the java process */
  jname = jprocess_get_name(jprocess_arg, env);

  if (!jname) {
    jxbt_throw_null(env,
                    xbt_strdup
                    ("Internal error: Process name cannot be NULL"));
    return;
  }

  /* allocate the data of the simulation */
  process = xbt_new0(s_m_process_t, 1);
  process->simdata = xbt_new0(s_simdata_process_t, 1);

  /* create a global java process instance */
  jprocess = jprocess_new_global_ref(jprocess_arg, env);

  if (!jprocess) {
    free(process->simdata);
    free(process);
    jxbt_throw_jni(env, "Can't get a global ref to the java process");
    return;
  }

  /* bind the java process instance to the native process */
  jprocess_bind(jprocess, process, env);

  /* build the C name of the process */
  name = (*env)->GetStringUTFChars(env, jname, 0);
  process->name = xbt_strdup(name);
  (*env)->ReleaseStringUTFChars(env, jname, name);

  process->simdata->m_host = jhost_get_native(env, jhost);


  if (!(process->simdata->m_host)) {    /* not binded */
    free(process->simdata);
    free(process->data);
    free(process);
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }
  process->simdata->PID = msg_global->PID++;

  /* create a new context */
  DEBUG8
    ("fill in process %s/%s (pid=%d) %p (sd=%p, host=%p, host->sd=%p); env=%p",
     process->name, process->simdata->m_host->name, process->simdata->PID,
     process, process->simdata, process->simdata->m_host,
     process->simdata->m_host->simdata, env);

  process->simdata->s_process = 
    SIMIX_process_create(process->name, (xbt_main_func_t)jprocess, 
                         /*data */ (void *) process,
                         process->simdata->m_host->simdata->smx_host->name, 
                         0, NULL, NULL);
    
  DEBUG1("context created (s_process=%p)", process->simdata->s_process);


  if (SIMIX_process_self()) {   /* someone created me */
    process->simdata->PPID = MSG_process_get_PID(SIMIX_process_self()->data);
  } else {
    process->simdata->PPID = -1;
  }

  process->simdata->last_errno = MSG_OK;

  /* add the process to the list of the processes of the simulation */
  xbt_fifo_unshift(msg_global->process_list, process);

  sprintf(alias, "%s:%s", (process->simdata->m_host->simdata->smx_host)->name,
          process->name);

  mailbox = MSG_mailbox_new(alias);
  
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processSuspend(JNIEnv * env, jclass cls,
                                          jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* try to suspend the process */
  MSG_error_t rv = MSG_process_suspend(process);
  
  jxbt_check_res("MSG_process_suspend()",rv,MSG_OK,
    bprintf("unexpected error , please report this bug"));
  
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processResume(JNIEnv * env, jclass cls,
                                         jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env,"process", jprocess);
    return;
  }

  /* try to resume the process */
    MSG_error_t rv = MSG_process_resume(process);
    
    jxbt_check_res("MSG_process_resume()",rv,MSG_OK,
    bprintf("unexpected error , please report this bug"));
}

JNIEXPORT jboolean JNICALL
Java_simgrid_msg_MsgNative_processIsSuspended(JNIEnv * env, jclass cls,
                                              jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  /* true is the process is suspended, false otherwise */
  return (jboolean) MSG_process_is_suspended(process);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processKill(JNIEnv * env, jclass cls,
                                       jobject jprocess)
{
  /* get the native instances from the java ones */
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* delete the global reference */
  jprocess_delete_global_ref(native_to_java_process(process), env);

  /* kill the native process (this wrapper is call by the destructor of the java 
   * process instance)
   */
  MSG_process_kill(process);
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_processGetHost(JNIEnv * env, jclass cls,
                                          jobject jprocess)
{
  /* get the native instances from the java ones */
  m_process_t process = jprocess_to_native_process(jprocess, env);
  m_host_t host;

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return NULL;
  }

  host = MSG_process_get_host(process);

  if (!MSG_host_get_data(host)) {
    jxbt_throw_jni(env, "MSG_process_get_host() failed");
    return NULL;
  }

  /* return the global reference to the java host instance */
  return (jobject) MSG_host_get_data(host);

}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_processFromPID(JNIEnv * env, jclass cls, jint PID)
{
  m_process_t process = MSG_process_from_PID(PID);

  if (!process) {
    jxbt_throw_process_not_found(env, bprintf("PID = %d", PID));
    return NULL;
  }

  if (!native_to_java_process(process)) {
    jxbt_throw_jni(env, "SIMIX_process_get_jprocess() failed");
    return NULL;
  }

  return (jobject) (native_to_java_process(process));
}


JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_processGetPID(JNIEnv * env, jclass cls,
                                         jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  return (jint) MSG_process_get_PID(process);
}


JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_processGetPPID(JNIEnv * env, jclass cls,
                                          jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  return (jint) MSG_process_get_PPID(process);
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_processSelf(JNIEnv * env, jclass cls)
{
  m_process_t process = MSG_process_self();
  jobject jprocess;

  if (!process) {
    jxbt_throw_jni(env, xbt_strdup("MSG_process_self() failed"));
    return NULL;
  }

  jprocess = native_to_java_process(process);

  if (!jprocess)
    jxbt_throw_jni(env, xbt_strdup("SIMIX_process_get_jprocess() failed"));

  return jprocess;
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processChangeHost(JNIEnv * env, jclass cls,
                                             jobject jhost)
{
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }

  /* try to change the host of the process */
  MSG_error_t rv = MSG_process_change_host(host);

  jxbt_check_res("MSG_process_change_host()",rv,MSG_OK,
      bprintf("unexpected error , please report this bug"));

}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processWaitFor(JNIEnv * env, jclass cls,
                                          jdouble seconds)
{
    MSG_error_t rv= MSG_process_sleep((double) seconds);
    
    jxbt_check_res("MSG_process_sleep()",rv, MSG_HOST_FAILURE,
    bprintf("while process was waiting for %f seconds",(double)seconds));
    
}


/***************************************************************************************
 * The MSG host connected functions implementation.                                    *
 ***************************************************************************************/

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_hostGetByName(JNIEnv * env, jclass cls,
                                         jstring jname)
{
  m_host_t host;                /* native host                                          */
  jobject jhost;                /* global reference to the java host instance returned  */

  /* get the C string from the java string */
  const char *name = (*env)->GetStringUTFChars(env, jname, 0);

  /* get the host by name       (the hosts are created during the grid resolution) */
  host = MSG_get_host_by_name(name);
  DEBUG2("MSG gave %p as native host (simdata=%p)", host, host->simdata);

  (*env)->ReleaseStringUTFChars(env, jname, name);

  if (!host) {                  /* invalid name */
    jxbt_throw_host_not_found(env, name);
    return NULL;
  }

  if (!MSG_host_get_data(host)) {            /* native host not associated yet with java host */

    /* instanciate a new java host */
    jhost = jhost_new_instance(env);

    if (!jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return NULL;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (!jhost) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return NULL;
    }

    /* bind the java host and the native host */
    jhost_bind(jhost, host, env);

    /* the native host data field is set with the global reference to the 
     * java host returned by this function 
     */
    MSG_host_set_data(host,(void *) jhost);
  }

  /* return the global reference to the java host instance */
  return (jobject) MSG_host_get_data(host);
}

JNIEXPORT jstring JNICALL
Java_simgrid_msg_MsgNative_hostGetName(JNIEnv * env, jclass cls,
                                       jobject jhost)
{
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return NULL;
  }

  return (*env)->NewStringUTF(env, MSG_host_get_name(host));
}

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_hostGetNumber(JNIEnv * env, jclass cls)
{
  return (jint) MSG_get_host_number();
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_hostSelf(JNIEnv * env, jclass cls)
{
  jobject jhost;

  m_host_t host = MSG_host_self();

  if (!MSG_host_get_data(host)) {
    /* the native host not yet associated with the java host instance */

    /* instanciate a new java host instance */
    jhost = jhost_new_instance(env);

    if (!jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return NULL;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (!jhost) {
      jxbt_throw_jni(env, "global ref allocation failed");
      return NULL;
    }

    /* Bind & store it */
    jhost_bind(jhost, host, env);
    MSG_host_set_data(host,(void *) jhost);
  } else {
    jhost = (jobject) MSG_host_get_data(host);
  }

  return jhost;
}

JNIEXPORT jdouble JNICALL
Java_simgrid_msg_MsgNative_hostGetSpeed(JNIEnv * env, jclass cls,
                                        jobject jhost)
{
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_get_host_speed(host);
}

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_hostGetLoad(JNIEnv * env, jclass cls,
                                       jobject jhost)
{
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jint) MSG_get_host_msgload(host);
}


JNIEXPORT jboolean JNICALL
Java_simgrid_msg_MsgNative_hostIsAvail(JNIEnv * env, jclass cls,
                                       jobject jhost)
{
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return (jboolean) MSG_host_is_avail(host);
}


/***************************************************************************************
 * The MSG task connected functions implementation.                                    *
 ***************************************************************************************/

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskCreate(JNIEnv * env, jclass cls, jobject jtask,
                                      jstring jname, jdouble jcomputeDuration,
                                      jdouble jmessageSize)
{
  m_task_t task;                /* the native task to create                            */
  const char *name=NULL;        /* the name of the task                                 */

  if (jcomputeDuration < 0) {
    jxbt_throw_illegal(env,
                       bprintf("Task ComputeDuration (%f) cannot be negative",
                               (double) jcomputeDuration));
    return;
  }

  if (jmessageSize < 0) {
    jxbt_throw_illegal(env,
                       bprintf("Task MessageSize (%f) cannot be negative",
                               (double) jmessageSize));
    return;
  }

  if (jname) {
    /* get the C string from the java string */
    name = (*env)->GetStringUTFChars(env, jname, 0);
  }


  /* create the task */
  task =
    MSG_task_create(name, (double) jcomputeDuration, (double) jmessageSize,
                    NULL);

  if (jname)
    (*env)->ReleaseStringUTFChars(env, jname, name);

  /* bind & store the task */
  jtask_bind(jtask, task, env);
  MSG_task_set_data(task,jtask);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_parallel_taskCreate(JNIEnv * env, jclass cls,
                                               jobject jtask, jstring jname,
                                               jobjectArray jhosts,
                                               jdoubleArray
                                               jcomputeDurations_arg,
                                               jdoubleArray jmessageSizes_arg)
{

  m_task_t task;                /* the native parallel task to create           */
  const char *name;             /* the name of the task                         */
  int host_count;
  m_host_t *hosts;
  double *computeDurations;
  double *messageSizes;
  jdouble *jcomputeDurations;
  jdouble *jmessageSizes;

  jobject jhost;
  int index;


  if (!jcomputeDurations_arg) {
    jxbt_throw_null(env,
                    xbt_strdup
                    ("Parallel task compute durations cannot be null"));
    return;
  }

  if (!jmessageSizes_arg) {
    jxbt_throw_null(env,
                    xbt_strdup("Parallel task message sizes cannot be null"));
    return;
  }

  if (!jname) {
    jxbt_throw_null(env, xbt_strdup("Parallel task name cannot be null"));
    return;
  }

  host_count = (int) (*env)->GetArrayLength(env, jhosts);


  hosts = xbt_new0(m_host_t, host_count);
  computeDurations = xbt_new0(double, host_count);
  messageSizes = xbt_new0(double, host_count * host_count);

  jcomputeDurations =
    (*env)->GetDoubleArrayElements(env, jcomputeDurations_arg, 0);
  jmessageSizes = (*env)->GetDoubleArrayElements(env, jmessageSizes_arg, 0);

  for (index = 0; index < host_count; index++) {
    jhost = (*env)->GetObjectArrayElement(env, jhosts, index);
    hosts[index] = jhost_get_native(env, jhost);
    computeDurations[index] = jcomputeDurations[index];
  }
  for (index = 0; index < host_count * host_count; index++) {
    messageSizes[index] = jmessageSizes[index];
  }

  (*env)->ReleaseDoubleArrayElements(env, jcomputeDurations_arg,
                                     jcomputeDurations, 0);
  (*env)->ReleaseDoubleArrayElements(env, jmessageSizes_arg, jmessageSizes,
                                     0);


  /* get the C string from the java string */
  name = (*env)->GetStringUTFChars(env, jname, 0);

  task =
    MSG_parallel_task_create(name, host_count, hosts, computeDurations,
                             messageSizes, NULL);

  (*env)->ReleaseStringUTFChars(env, jname, name);

  /* associate the java task object and the native task */
  jtask_bind(jtask, task, env);

  MSG_task_set_data(task,(void*) jtask);

  if (!MSG_task_get_data(task))
    jxbt_throw_jni(env, "global ref allocation failed");
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_taskGetSender(JNIEnv * env, jclass cls,
                                         jobject jtask)
{
  m_process_t process;

  m_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return NULL;
  }

  process = MSG_task_get_sender(task);
  return (jobject) native_to_java_process(process);
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_taskGetSource(JNIEnv * env, jclass cls,
                                         jobject jtask)
{
  m_host_t host;
  m_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return NULL;
  }

  host = MSG_task_get_source(task);

  if (!MSG_host_get_data(host)) {
    jxbt_throw_jni(env, "MSG_task_get_source() failed");
    return NULL;
  }

  return (jobject) MSG_host_get_data(host);
}


JNIEXPORT jstring JNICALL
Java_simgrid_msg_MsgNative_taskGetName(JNIEnv * env, jclass cls,
                                       jobject jtask)
{
  m_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return NULL;
  }

  return (*env)->NewStringUTF(env, MSG_task_get_name(task));
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskCancel(JNIEnv * env, jclass cls, jobject jtask)
{
  m_task_t ptask = jtask_to_native_task(jtask, env);

  if (!ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  MSG_error_t rv = MSG_task_cancel(ptask);
  
    jxbt_check_res("MSG_task_cancel()",rv,MSG_OK,
    bprintf("unexpected error , please report this bug"));
}

JNIEXPORT jdouble JNICALL
Java_simgrid_msg_MsgNative_taskGetComputeDuration(JNIEnv * env, jclass cls,
                                                  jobject jtask)
{
  m_task_t ptask = jtask_to_native_task(jtask, env);

  if (!ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return -1;
  }
  return (jdouble) MSG_task_get_compute_duration(ptask);
}

JNIEXPORT jdouble JNICALL
Java_simgrid_msg_MsgNative_taskGetRemainingDuration(JNIEnv * env, jclass cls,
                                                    jobject jtask)
{
  m_task_t ptask = jtask_to_native_task(jtask, env);

  if (!ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return -1;
  }
  return (jdouble) MSG_task_get_remaining_computation(ptask);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskSetPriority(JNIEnv * env, jclass cls,
                                           jobject jtask, jdouble priority)
{
  m_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_priority(task, (double) priority);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskDestroy(JNIEnv * env, jclass cls,
                                       jobject jtask_arg)
{

  /* get the native task */
  m_task_t task = jtask_to_native_task(jtask_arg, env);
  jobject jtask;

  if (!task) {
    jxbt_throw_notbound(env, "task", task);
    return;
  }
  jtask = (jobject) MSG_task_get_data(task);
    
  MSG_error_t rv = MSG_task_destroy(task);
    
  jxbt_check_res("MSG_task_destroy()",rv,MSG_OK,
    bprintf("unexpected error , please report this bug"));
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskExecute(JNIEnv * env, jclass cls,
                                       jobject jtask)
{
  m_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  MSG_error_t rv = MSG_task_execute(task);
  
    jxbt_check_res("MSG_task_execute()",rv,MSG_HOST_FAILURE|MSG_TASK_CANCELLED,
    bprintf("while executing task %s", MSG_task_get_name(task)));
}

/***************************************************************************************
 * Unsortable functions                                                        *
 ***************************************************************************************/


JNIEXPORT jint JNICALL
Java_simgrid_msg_Msg_getErrCode(JNIEnv * env, jclass cls)
{
  return (jint) MSG_get_errno();
}

JNIEXPORT jdouble JNICALL
Java_simgrid_msg_Msg_getClock(JNIEnv * env, jclass cls)
{
  return (jdouble) MSG_get_clock();
}


JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_init(JNIEnv * env, jclass cls, jobjectArray jargs) {
  char **argv = NULL;
  int index;
  int argc = 0;
  jstring jval;
  const char *tmp;

  if (jargs)
    argc = (int) (*env)->GetArrayLength(env, jargs);

  argc++;
  argv = xbt_new0(char *, argc);
  argv[0] = strdup("java");

  for (index = 0; index < argc - 1; index++) {
    jval = (jstring) (*env)->GetObjectArrayElement(env, jargs, index);
    tmp = (*env)->GetStringUTFChars(env, jval, 0);
    argv[index + 1] = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jval, tmp);
  }

  MSG_global_init(&argc, argv);
  SIMIX_context_select_factory("java");

  for (index = 0; index < argc; index++)
    free(argv[index]);

  free(argv);

  (*env)->GetJavaVM(env, &__java_vm);
}

JNIEXPORT void JNICALL
  JNICALL Java_simgrid_msg_Msg_run(JNIEnv * env, jclass cls) {
  MSG_error_t rv;
  int index;//xbt_fifo_item_t item = NULL;
  m_host_t *hosts;
  jobject jhost;

  /* Run everything */
  rv= MSG_main();
    jxbt_check_res("MSG_main()",rv,MSG_OK,
     bprintf("unexpected error : MSG_main() failed .. please report this bug "));

  DEBUG0
    ("MSG_main finished. Bail out before cleanup since there is a bug in this part.");

  DEBUG0("Clean java world");
  /* Cleanup java hosts */
  hosts = MSG_get_host_table();
  for (index=0;index<MSG_get_host_number()-1;index++)
  {
    jhost = (jobject)hosts[index]->data;
    if(jhost)
    	jhost_unref(env,jhost);

  }

  DEBUG0("Clean native world");
  /* cleanup native stuff */
  rv = MSG_OK != MSG_clean();
  jxbt_check_res("MSG_clean()",rv,MSG_OK,
       bprintf("unexpected error : MSG_clean() failed .. please report this bug "));

}

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_processKillAll(JNIEnv * env, jclass cls,
                                          jint jresetPID)
{
  return (jint) MSG_process_killall((int) jresetPID);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_createEnvironment(JNIEnv * env, jclass cls,
                                       jstring jplatformFile)
{

  const char *platformFile = (*env)->GetStringUTFChars(env, jplatformFile, 0);

  MSG_create_environment(platformFile);

  (*env)->ReleaseStringUTFChars(env, jplatformFile, platformFile);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processExit(JNIEnv * env, jclass cls,
                                       jobject jprocess)
{

  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  SIMIX_context_stop(SIMIX_process_self()->context);
}

JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_info(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  INFO1("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}

JNIEXPORT jobjectArray JNICALL
Java_simgrid_msg_MsgNative_allHosts(JNIEnv * env, jclass cls_arg)
{
  int index;
  jobjectArray jtable;
  jobject jhost;
  jstring jname;
  m_host_t host;

  int count = MSG_get_host_number();
  m_host_t *table = MSG_get_host_table();

  jclass cls = jxbt_get_class(env, "simgrid/msg/Host");

  if (!cls) {
    return NULL;
  }

  jtable = (*env)->NewObjectArray(env, (jsize) count, cls, NULL);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return NULL;
  }

  for (index = 0; index < count; index++) {
    host = table[index];
    jhost = (jobject) (MSG_host_get_data(host));

    if (!jhost) {
      jname = (*env)->NewStringUTF(env, MSG_host_get_name(host));

      jhost = Java_simgrid_msg_MsgNative_hostGetByName(env, cls_arg, jname);
      /* FIXME: leak of jname ? */
    }

    (*env)->SetObjectArrayElement(env, jtable, index, jhost);
  }

  return jtable;
}


JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_selectContextFactory(JNIEnv * env, jclass class,
                                                jstring jname)
{
  char *errmsg = NULL;
  xbt_ex_t e;

  /* get the C string from the java string */
  const char *name = (*env)->GetStringUTFChars(env, jname, 0);

  TRY {
    SIMIX_context_select_factory(name);
  } CATCH(e) {
    errmsg = xbt_strdup(e.msg);
    xbt_ex_free(e);
  }

  (*env)->ReleaseStringUTFChars(env, jname, name);

  if (errmsg) {
    char *thrown = bprintf("xbt_select_context_factory() failed: %s", errmsg);
    free(errmsg);
    jxbt_throw_jni(env, thrown);
  }
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskSend(JNIEnv * env, jclass cls,
                                    jstring jalias, jobject jtask,
                                    jdouble jtimeout)
{

  MSG_error_t rv;
  const char *alias = (*env)->GetStringUTFChars(env, jalias, 0);

  m_task_t task = jtask_to_native_task(jtask, env);


  if (!task) {
    (*env)->ReleaseStringUTFChars(env, jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task,(void *) (*env)->NewGlobalRef(env, jtask));
  rv = MSG_task_send_with_timeout(task, alias, (double) jtimeout);

  (*env)->ReleaseStringUTFChars(env, jalias, alias);

  jxbt_check_res("MSG_task_send_with_timeout()",rv, MSG_HOST_FAILURE|MSG_TRANSFER_FAILURE|MSG_TIMEOUT,
    bprintf("while sending task %s to mailbox %s", MSG_task_get_name(task),alias));
}

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskSendBounded(JNIEnv * env, jclass cls,
                                           jstring jalias, jobject jtask,
                                           jdouble jmaxRate)
{
  m_task_t task = jtask_to_native_task(jtask, env);
  MSG_error_t rv;
  const char *alias;

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  alias = (*env)->GetStringUTFChars(env, jalias, 0);

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task,(void *) (*env)->NewGlobalRef(env, jtask));
  rv = MSG_task_send_bounded(task, alias, (double) jmaxRate);

  (*env)->ReleaseStringUTFChars(env, jalias, alias);

  jxbt_check_res("MSG_task_send_bounded()",rv, MSG_HOST_FAILURE|MSG_TRANSFER_FAILURE|MSG_TIMEOUT,
    bprintf("while sending task %s to mailbox %s with max rate %f", MSG_task_get_name(task),alias,(double)jmaxRate));
    
}

JNIEXPORT jobject JNICALL
Java_simgrid_msg_MsgNative_taskReceive(JNIEnv * env, jclass cls,
                                       jstring jalias, jdouble jtimeout,
                                       jobject jhost)
{
  MSG_error_t rv;
  m_task_t task = NULL;
  m_host_t host = NULL;
  jobject jtask_global, jtask_local;
  const char *alias;

  if (jhost) {
    host = jhost_get_native(env, jhost);

    if (!host) {
      jxbt_throw_notbound(env, "host", jhost);
      return NULL;
    }
  }

  alias = (*env)->GetStringUTFChars(env, jalias, 0);

  rv = MSG_task_receive_ext(&task, alias, (double) jtimeout, host);
  jtask_global = MSG_task_get_data(task);

  /* Convert the global ref into a local ref so that the JVM can free the stuff */
  jtask_local = (*env)->NewLocalRef(env, jtask_global);
  (*env)->DeleteGlobalRef(env, jtask_global);
  MSG_task_set_data(task,NULL);

  (*env)->ReleaseStringUTFChars(env, jalias, alias);

  jxbt_check_res("MSG_task_receive_ext()",rv, MSG_HOST_FAILURE|MSG_TRANSFER_FAILURE|MSG_TIMEOUT,
    bprintf("while receiving from mailbox %s",alias));

  return (jobject) jtask_local;
}

JNIEXPORT jboolean JNICALL
Java_simgrid_msg_MsgNative_taskListen(JNIEnv * env, jclass cls,
                                      jstring jalias)
{

  const char *alias;
  int rv;

  alias = (*env)->GetStringUTFChars(env, jalias, 0);

  rv = MSG_task_listen(alias);

  (*env)->ReleaseStringUTFChars(env, jalias, alias);

  return (jboolean) rv;
}

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_taskListenFromHost(JNIEnv * env, jclass cls,
                                              jstring jalias, jobject jhost)
{
  int rv;
  const char *alias;

  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }
  alias = (*env)->GetStringUTFChars(env, jalias, 0);

  rv = MSG_task_listen_from_host(alias, host);

  (*env)->ReleaseStringUTFChars(env, jalias, alias);

  return (jint) rv;
}

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_taskListenFrom(JNIEnv * env, jclass cls,
                                          jstring jalias)
{

  int rv;
  const char *alias = (*env)->GetStringUTFChars(env, jalias, 0);

  rv = MSG_task_listen_from(alias);

  (*env)->ReleaseStringUTFChars(env, jalias, alias);

  return (jint) rv;
}

JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_deployApplication(JNIEnv * env, jclass cls,
                                       jstring jdeploymentFile)
{

  const char *deploymentFile =
    (*env)->GetStringUTFChars(env, jdeploymentFile, 0);

  surf_parse_reset_parser();

  surfxml_add_callback(STag_surfxml_process_cb_list,
                       japplication_handler_on_begin_process);

  surfxml_add_callback(ETag_surfxml_argument_cb_list,
                       japplication_handler_on_process_arg);

  surfxml_add_callback(STag_surfxml_prop_cb_list,
                       japplication_handler_on_property);

  surfxml_add_callback(ETag_surfxml_process_cb_list,
                       japplication_handler_on_end_process);

  surf_parse_open(deploymentFile);

  japplication_handler_on_start_document();

  if (surf_parse())
    jxbt_throw_jni(env,"surf_parse() failed");

  surf_parse_close();

  japplication_handler_on_end_document();

  (*env)->ReleaseStringUTFChars(env, jdeploymentFile, deploymentFile);
}
