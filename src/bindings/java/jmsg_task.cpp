/* Functions related to the java task instances.                            */

/* Copyright (c) 2007, 2009-2010, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg.h"
#include "jmsg_host.h"
#include "jmsg_task.h"
#include "jmsg_process.h"
#include "jxbt_utilities.h"

#include <simgrid/msg.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jmethodID jtask_method_Comm_constructor;

static jfieldID jtask_field_Task_bind;
static jfieldID jtask_field_Task_name;
static jfieldID jtask_field_Task_messageSize;
static jfieldID jtask_field_Comm_bind;
static jfieldID jtask_field_Comm_taskBind;
static jfieldID jtask_field_Comm_receiving;

void jtask_bind(jobject jtask, msg_task_t task, JNIEnv * env)
{
  env->SetLongField(jtask, jtask_field_Task_bind, (intptr_t)task);
}

msg_task_t jtask_to_native_task(jobject jtask, JNIEnv * env)
{
  return (msg_task_t)(intptr_t)env->GetLongField(jtask, jtask_field_Task_bind);
}

jboolean jtask_is_valid(jobject jtask, JNIEnv * env)
{
  return env->GetLongField(jtask, jtask_field_Task_bind) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeInit(JNIEnv *env, jclass cls) {
  jclass jtask_class_Comm = env->FindClass("org/simgrid/msg/Comm");
  jclass jtask_class_Task = env->FindClass("org/simgrid/msg/Task");

  jtask_method_Comm_constructor = env->GetMethodID(jtask_class_Comm, "<init>", "()V");
  jtask_field_Task_bind = jxbt_get_jfield(env, jtask_class_Task, "bind", "J");
  jtask_field_Task_name = jxbt_get_jfield(env, jtask_class_Task, "name", "Ljava/lang/String;");
  jtask_field_Task_messageSize = jxbt_get_jfield(env, jtask_class_Task, "messageSize", "D");
  jtask_field_Comm_bind = jxbt_get_jfield(env, jtask_class_Comm, "bind", "J");
  jtask_field_Comm_taskBind = jxbt_get_jfield(env, jtask_class_Comm, "taskBind", "J");
  jtask_field_Comm_receiving = jxbt_get_jfield(env, jtask_class_Comm, "receiving", "Z");
  if (!jtask_field_Task_bind || !jtask_class_Task || !jtask_field_Comm_bind || !jtask_field_Comm_taskBind ||
        !jtask_field_Comm_receiving || !jtask_method_Comm_constructor) {
          jxbt_throw_native(env,bprintf("Can't find some fields in Java class."));
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_create(JNIEnv * env, jobject jtask, jstring jname,
                                      jdouble jflopsAmount, jdouble jbytesAmount)
{
  msg_task_t task;                /* the native task to create                            */
  const char *name = NULL;      /* the name of the task                                 */

  if (jflopsAmount < 0) {
    jxbt_throw_illegal(env, bprintf("Task flopsAmount (%f) cannot be negative", (double) jflopsAmount));
    return;
  }

  if (jbytesAmount < 0) {
    jxbt_throw_illegal(env, bprintf("Task bytesAmount (%f) cannot be negative", (double) jbytesAmount));
    return;
  }

  if (jname) {
    /* get the C string from the java string */
    name = env->GetStringUTFChars(jname, 0);
  }

  /* create the task */
  task = MSG_task_create(name, (double) jflopsAmount, (double) jbytesAmount, NULL);
  if (jname)
    env->ReleaseStringUTFChars(jname, name);
  /* sets the task name */
  env->SetObjectField(jtask, jtask_field_Task_name, jname);
  /* bind & store the task */
  jtask_bind(jtask, task, env);
  MSG_task_set_data(task, jtask);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_parallelCreate(JNIEnv * env, jobject jtask, jstring jname,
                                         jobjectArray jhosts, jdoubleArray jcomputeDurations_arg,
                                         jdoubleArray jmessageSizes_arg)
{
  msg_task_t task;                /* the native parallel task to create           */
  const char *name;             /* the name of the task                         */
  int host_count;
  msg_host_t *hosts;
  double *computeDurations;
  double *messageSizes;
  jdouble *jcomputeDurations;
  jdouble *jmessageSizes;
  jobject jhost;
  int index;

  if (!jcomputeDurations_arg) {
    jxbt_throw_null(env, xbt_strdup("Parallel task flops amounts cannot be null"));
    return;
  }

  if (!jmessageSizes_arg) {
    jxbt_throw_null(env, xbt_strdup("Parallel task bytes amounts cannot be null"));
    return;
  }

  if (!jname) {
    jxbt_throw_null(env, xbt_strdup("Parallel task name cannot be null"));
    return;
  }

  host_count = (int) env->GetArrayLength(jhosts);

  hosts = xbt_new0(msg_host_t, host_count);
  computeDurations = xbt_new0(double, host_count);
  messageSizes = xbt_new0(double, host_count * host_count);

  jcomputeDurations = env->GetDoubleArrayElements(jcomputeDurations_arg, 0);
  jmessageSizes = env->GetDoubleArrayElements(jmessageSizes_arg, 0);

  for (index = 0; index < host_count; index++) {
    jhost = env->GetObjectArrayElement(jhosts, index);
    hosts[index] = jhost_get_native(env, jhost);
    computeDurations[index] = jcomputeDurations[index];
  }
  for (index = 0; index < host_count * host_count; index++) {
    messageSizes[index] = jmessageSizes[index];
  }

  env->ReleaseDoubleArrayElements(jcomputeDurations_arg, jcomputeDurations, 0);
  env->ReleaseDoubleArrayElements(jmessageSizes_arg, jmessageSizes, 0);

  /* get the C string from the java string */
  name = env->GetStringUTFChars(jname, 0);

  task = MSG_parallel_task_create(name, host_count, hosts, computeDurations, messageSizes, NULL);

  env->ReleaseStringUTFChars(jname, name);
  /* sets the task name */
  env->SetObjectField(jtask, jtask_field_Task_name, jname);
  /* associate the java task object and the native task */
  jtask_bind(jtask, task, env);

  MSG_task_set_data(task, (void *) jtask);

  if (!MSG_task_get_data(task))
    jxbt_throw_jni(env, "global ref allocation failed");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_cancel(JNIEnv * env, jobject jtask)
{
  msg_task_t ptask = jtask_to_native_task(jtask, env);

  if (!ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  msg_error_t rv = MSG_task_cancel(ptask);

  jxbt_check_res("MSG_task_cancel()", rv, MSG_OK, bprintf("unexpected error , please report this bug"));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_execute(JNIEnv * env, jobject jtask)
{
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  msg_error_t rv;
  rv = MSG_task_execute(task);
  if (env->ExceptionOccurred())
    return;
  if (rv != MSG_OK) {
    jmsg_throw_status(env, rv);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBound(JNIEnv * env, jobject jtask, jdouble bound)
{
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_bound(task, bound);
}

JNIEXPORT jstring JNICALL Java_org_simgrid_msg_Task_getName(JNIEnv * env, jobject jtask) {
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return NULL;
  }

  return env->NewStringUTF(MSG_task_get_name(task));
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSender(JNIEnv * env, jobject jtask) {
  msg_process_t process;

  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return NULL;
  }

  process = MSG_task_get_sender(task);
  if (process == NULL) {
    return NULL;
  }
  return (jobject) native_to_java_process(process);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSource(JNIEnv * env, jobject jtask)
{
  msg_host_t host;
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return NULL;
  }

  host = MSG_task_get_source(task);
  if (host == NULL) {
    return NULL;
  }
  if (!host->extension(JAVA_HOST_LEVEL)) {
    jxbt_throw_jni(env, "MSG_task_get_source() failed");
    return NULL;
  }

  return (jobject) host->extension(JAVA_HOST_LEVEL);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Task_getFlopsAmount(JNIEnv * env, jobject jtask)
{
  msg_task_t ptask = jtask_to_native_task(jtask, env);

  if (!ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return -1;
  }
  return (jdouble) MSG_task_get_flops_amount(ptask);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setName(JNIEnv *env, jobject jtask, jobject jname) {
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);

  env->SetObjectField(jtask, jtask_field_Task_name, jname);
  MSG_task_set_name(task, name);

  env->ReleaseStringUTFChars((jstring) jname, name);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setPriority(JNIEnv * env, jobject jtask, jdouble priority)
{
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_priority(task, (double) priority);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setFlopsAmount (JNIEnv *env, jobject jtask, jdouble computationAmount)
{
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_flops_amount(task, (double) computationAmount);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBytesAmount (JNIEnv *env, jobject jtask, jdouble dataSize)
{
  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  env->SetDoubleField(jtask, jtask_field_Task_messageSize, dataSize);
  MSG_task_set_bytes_amount(task, (double) dataSize);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_send(JNIEnv * env,jobject jtask, jstring jalias, jdouble jtimeout)
{
  msg_error_t rv;
  const char *alias = env->GetStringUTFChars(jalias, 0);

  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    env->ReleaseStringUTFChars(jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, (void *) env->NewGlobalRef(jtask));
  rv = MSG_task_send_with_timeout(task, alias, (double) jtimeout);
  env->ReleaseStringUTFChars(jalias, alias);

  if (rv != MSG_OK) {
    jmsg_throw_status(env, rv);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_sendBounded(JNIEnv * env,jobject jtask, jstring jalias,
                                                             jdouble jtimeout,jdouble maxrate)
{
  msg_error_t rv;
  const char *alias = env->GetStringUTFChars(jalias, 0);

  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    env->ReleaseStringUTFChars(jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, (void *) env->NewGlobalRef(jtask));
  rv = MSG_task_send_with_timeout_bounded(task, alias, (double) jtimeout, (double) maxrate);
  env->ReleaseStringUTFChars(jalias, alias);

  if (rv != MSG_OK) {
    jmsg_throw_status(env, rv);
  }
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receive(JNIEnv * env, jclass cls, jstring jalias, jdouble jtimeout,
                                                            jobject jhost)
{
  msg_error_t rv;
  msg_task_t *task = xbt_new(msg_task_t,1);
  *task = NULL;

  msg_host_t host = NULL;
  jobject jtask_global, jtask_local;
  const char *alias;

  if (jhost) {
    host = jhost_get_native(env, jhost);

    if (!host) {
      jxbt_throw_notbound(env, "host", jhost);
      return NULL;
    }
  }

  alias = env->GetStringUTFChars(jalias, 0);
  rv = MSG_task_receive_ext(task, alias, (double) jtimeout, host);
  if (env->ExceptionOccurred())
    return NULL;
  if (rv != MSG_OK) {
    jmsg_throw_status(env,rv);
    return NULL;
  }
  jtask_global = (jobject) MSG_task_get_data(*task);

  /* Convert the global ref into a local ref so that the JVM can free the stuff */
  jtask_local = env->NewLocalRef(jtask_global);
  env->DeleteGlobalRef(jtask_global);
  MSG_task_set_data(*task, NULL);

  env->ReleaseStringUTFChars(jalias, alias);

  xbt_free(task);

  return (jobject) jtask_local;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecv(JNIEnv * env, jclass cls, jstring jmailbox) {
  msg_comm_t comm;
  const char *mailbox;
  jclass comm_class;
  //pointer to store the task object pointer.
  msg_task_t *task = xbt_new(msg_task_t,1);
  *task = NULL;
  /* There should be a cache here */
  comm_class = env->FindClass("org/simgrid/msg/Comm");

  if (!comm_class) {
    jxbt_throw_native(env,bprintf("fieldID or methodID or class not found."));
    return NULL;
  }

  jobject jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  if (!jcomm) {
    jxbt_throw_native(env,bprintf("Can't create a Comm object."));
    return NULL;
  }

  mailbox = env->GetStringUTFChars(jmailbox, 0);

  comm = MSG_task_irecv(task,mailbox);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(task));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_TRUE);

  env->ReleaseStringUTFChars(jmailbox, mailbox);

  return jcomm;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receiveBounded(JNIEnv * env, jclass cls, jstring jalias,
                                                                   jdouble jtimeout, jobject jhost, jdouble rate)
{
  msg_error_t rv;
  msg_task_t *task = xbt_new(msg_task_t,1);
  *task = NULL;

  msg_host_t host = NULL;
  jobject jtask_global, jtask_local;
  const char *alias;

  if (jhost) {
    host = jhost_get_native(env, jhost);

    if (!host) {
      jxbt_throw_notbound(env, "host", jhost);
      return NULL;
    }
  }

  alias = env->GetStringUTFChars(jalias, 0);
  rv = MSG_task_receive_ext_bounded(task, alias, (double) jtimeout, host, (double) rate);
  if (env->ExceptionOccurred())
    return NULL;
  if (rv != MSG_OK) {
    jmsg_throw_status(env,rv);
    return NULL;
  }
  jtask_global = (jobject) MSG_task_get_data(*task);

  /* Convert the global ref into a local ref so that the JVM can free the stuff */
  jtask_local = env->NewLocalRef(jtask_global);
  env->DeleteGlobalRef(jtask_global);
  MSG_task_set_data(*task, NULL);

  env->ReleaseStringUTFChars(jalias, alias);

  xbt_free(task);

  return (jobject) jtask_local;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecvBounded(JNIEnv * env, jclass cls, jstring jmailbox,
                                                                 jdouble rate)
{
  msg_comm_t comm;
  const char *mailbox;
  jclass comm_class;
  //pointer to store the task object pointer.
  msg_task_t *task = xbt_new(msg_task_t,1);
  *task = NULL;
  /* There should be a cac hee */
  comm_class = env->FindClass("org/simgrid/msg/Comm");

  if (!comm_class) {
    jxbt_throw_native(env,bprintf("fieldID or methodID or class not found."));
    return NULL;
  }

  jobject jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  if (!jcomm) {
    jxbt_throw_native(env,bprintf("Can't create a Comm object."));
    return NULL;
  }

  mailbox = env->GetStringUTFChars(jmailbox, 0);

  comm = MSG_task_irecv_bounded(task,mailbox, (double) rate);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(task));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_TRUE);

  env->ReleaseStringUTFChars(jmailbox, mailbox);

  return jcomm;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isend(JNIEnv *env, jobject jtask, jstring jmailbox)
{
  jclass comm_class;
  msg_task_t task;
  jobject jcomm;
  msg_comm_t comm;
  const char *mailbox;

  comm_class = env->FindClass("org/simgrid/msg/Comm");

  if (!comm_class) return NULL;

  jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  mailbox = env->GetStringUTFChars(jmailbox, 0);

  task = jtask_to_native_task(jtask, env);

  if (!task) {
    env->ReleaseStringUTFChars(jmailbox, mailbox);
    env->DeleteLocalRef(jcomm);
    jxbt_throw_notbound(env, "task", jtask);
        return NULL;
  }

  MSG_task_set_data(task, (void *) env->NewGlobalRef(jtask));
  comm = MSG_task_isend(task,mailbox);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(NULL));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_FALSE);

  env->ReleaseStringUTFChars(jmailbox, mailbox);

  return jcomm;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isendBounded(JNIEnv *env, jobject jtask, jstring jmailbox,
                                                                 jdouble maxrate)
{
  jclass comm_class;
  msg_task_t task;
  jobject jcomm;
  msg_comm_t comm;
  const char *mailbox;

  comm_class = env->FindClass("org/simgrid/msg/Comm");

  if (!comm_class) return NULL;

  jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  mailbox = env->GetStringUTFChars(jmailbox, 0);

  task = jtask_to_native_task(jtask, env);

  if (!task) {
    env->ReleaseStringUTFChars(jmailbox, mailbox);
    env->DeleteLocalRef(jcomm);
    jxbt_throw_notbound(env, "task", jtask);
        return NULL;
  }

  MSG_task_set_data(task, (void *) env->NewGlobalRef(jtask));
  comm = MSG_task_isend_bounded(task,mailbox,maxrate);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(NULL));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_FALSE);

  env->ReleaseStringUTFChars(jmailbox, mailbox);

  return jcomm;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeFinalize(JNIEnv * env, jobject jtask)
{
    msg_task_t task = jtask_to_native_task(jtask, env);

    if (!task) {
      jxbt_throw_notbound(env, "task", jtask);
      return;
    }

    MSG_task_destroy(task);
}

static void msg_task_cancel_on_failed_dsend(void*t) {
  msg_task_t task = (msg_task_t) t;
  JNIEnv *env =get_current_thread_env();
  jobject jtask_global = (jobject) MSG_task_get_data(task);

  /* Destroy the global ref so that the JVM can free the stuff */
  env->DeleteGlobalRef(jtask_global);
  MSG_task_set_data(task, NULL);
  MSG_task_destroy(task);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsend(JNIEnv * env, jobject jtask, jstring jalias)
{
  const char *alias = env->GetStringUTFChars(jalias, 0);

  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    env->ReleaseStringUTFChars(jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, (void *) env->NewGlobalRef(jtask));
  MSG_task_dsend(task, alias, msg_task_cancel_on_failed_dsend);

  env->ReleaseStringUTFChars(jalias, alias);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsendBounded(JNIEnv * env, jobject jtask, jstring jalias,
                                                              jdouble maxrate)
{
  const char *alias = env->GetStringUTFChars(jalias, 0);

  msg_task_t task = jtask_to_native_task(jtask, env);

  if (!task) {
    env->ReleaseStringUTFChars(jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, (void *) env->NewGlobalRef(jtask));
  MSG_task_dsend_bounded(task, alias, msg_task_cancel_on_failed_dsend,(double)maxrate);

  env->ReleaseStringUTFChars(jalias, alias);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Task_listen(JNIEnv * env, jclass cls, jstring jalias)
{
  const char *alias;
  int rv;

  alias = env->GetStringUTFChars(jalias, 0);
  rv = MSG_task_listen(alias);
  env->ReleaseStringUTFChars(jalias, alias);

  return (jboolean) rv;
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFromHost(JNIEnv * env, jclass cls, jstring jalias, jobject jhost)
{
  int rv;
  const char *alias;

  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }
  alias = env->GetStringUTFChars(jalias, 0);
  rv = MSG_task_listen_from_host(alias, host);
  env->ReleaseStringUTFChars(jalias, alias);

  return (jint) rv;
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFrom(JNIEnv * env, jclass cls, jstring jalias)
{
  int rv;
  const char *alias = env->GetStringUTFChars(jalias, 0);
  rv = MSG_task_listen_from(alias);
  env->ReleaseStringUTFChars(jalias, alias);

  return (jint) rv;
}
