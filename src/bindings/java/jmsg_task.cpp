/* Functions related to the java task instances.                            */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/context/Context.hpp"

#include "jmsg.hpp"
#include "jmsg_host.h"
#include "jmsg_process.h"
#include "jmsg_task.h"
#include "jxbt_utilities.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

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

msg_task_t jtask_to_native(jobject jtask, JNIEnv* env)
{
  return (msg_task_t)(intptr_t)env->GetLongField(jtask, jtask_field_Task_bind);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeInit(JNIEnv *env, jclass cls) {
  jclass jtask_class_Comm = env->FindClass("org/simgrid/msg/Comm");
  jclass jtask_class_Task = env->FindClass("org/simgrid/msg/Task");
  xbt_assert(jtask_class_Comm && jtask_class_Task,
             "Native initialization of msg/Comm or msg/Task failed. Please report that bug");

  jtask_method_Comm_constructor = env->GetMethodID(jtask_class_Comm, "<init>", "()V");
  jtask_field_Task_bind = jxbt_get_jfield(env, jtask_class_Task, "bind", "J");
  jtask_field_Task_name = jxbt_get_jfield(env, jtask_class_Task, "name", "Ljava/lang/String;");
  jtask_field_Task_messageSize = jxbt_get_jfield(env, jtask_class_Task, "messageSize", "D");
  jtask_field_Comm_bind = jxbt_get_jfield(env, jtask_class_Comm, "bind", "J");
  jtask_field_Comm_taskBind = jxbt_get_jfield(env, jtask_class_Comm, "taskBind", "J");
  jtask_field_Comm_receiving = jxbt_get_jfield(env, jtask_class_Comm, "receiving", "Z");
  xbt_assert(jtask_field_Task_bind && jtask_field_Comm_bind && jtask_field_Comm_taskBind &&
                 jtask_field_Comm_receiving && jtask_method_Comm_constructor,
             "Native initialization of msg/Task failed. Please report that bug");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_create(JNIEnv * env, jobject jtask, jstring jname,
                                      jdouble jflopsAmount, jdouble jbytesAmount)
{
  const char *name = nullptr;      /* the name of the task                                 */

  if (jname)
    name = env->GetStringUTFChars(jname, nullptr);
  msg_task_t task = MSG_task_create(name, jflopsAmount, jbytesAmount, jtask);
  if (jname)
    env->ReleaseStringUTFChars(jname, name);

  /* bind & store the task */
  jtask_bind(jtask, task, env);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_parallelCreate(JNIEnv * env, jobject jtask, jstring jname,
                                         jobjectArray jhosts, jdoubleArray jcomputeDurations_arg,
                                         jdoubleArray jmessageSizes_arg)
{
  int host_count = env->GetArrayLength(jhosts);

  jdouble* jcomputeDurations = env->GetDoubleArrayElements(jcomputeDurations_arg, nullptr);
  auto* hosts                = new msg_host_t[host_count];
  auto* computeDurations     = new double[host_count];
  for (int index = 0; index < host_count; index++) {
    jobject jhost           = env->GetObjectArrayElement(jhosts, index);
    hosts[index] = jhost_get_native(env, jhost);
    computeDurations[index] = jcomputeDurations[index];
  }
  env->ReleaseDoubleArrayElements(jcomputeDurations_arg, jcomputeDurations, 0);

  jdouble* jmessageSizes = env->GetDoubleArrayElements(jmessageSizes_arg, nullptr);
  auto* messageSizes     = new double[host_count * host_count];
  for (int index = 0; index < host_count * host_count; index++) {
    messageSizes[index] = jmessageSizes[index];
  }
  env->ReleaseDoubleArrayElements(jmessageSizes_arg, jmessageSizes, 0);

  /* get the C string from the java string */
  const char* name = env->GetStringUTFChars(jname, nullptr);
  msg_task_t task  = MSG_parallel_task_create(name, host_count, hosts, computeDurations, messageSizes, jtask);
  env->ReleaseStringUTFChars(jname, name);

  /* associate the java task object and the native task */
  jtask_bind(jtask, task, env);

  delete[] hosts;
  delete[] computeDurations;
  delete[] messageSizes;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_cancel(JNIEnv * env, jobject jtask)
{
  msg_task_t ptask = jtask_to_native(jtask, env);

  if (not ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  msg_error_t rv = MSG_task_cancel(ptask);

  jxbt_check_res("MSG_task_cancel()", rv, MSG_OK, "unexpected error , please report this bug");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_execute(JNIEnv * env, jobject jtask)
{
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  msg_error_t rv;
  if (not simgrid::ForcefulKillException::try_n_catch([&rv, &task]() { rv = MSG_task_execute(task); })) {
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", "Process killed");
  }

  if (env->ExceptionOccurred())
    return;
  if (rv != MSG_OK) {
    jmsg_throw_status(env, rv);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBound(JNIEnv * env, jobject jtask, jdouble bound)
{
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_bound(task, bound);
}

JNIEXPORT jstring JNICALL Java_org_simgrid_msg_Task_getName(JNIEnv * env, jobject jtask) {
  const_msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return nullptr;
  }

  return env->NewStringUTF(MSG_task_get_name(task));
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSender(JNIEnv * env, jobject jtask) {
  const_msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return nullptr;
  }

  auto const* process = MSG_task_get_sender(task);
  if (process == nullptr) {
    return nullptr;
  }
  return (jobject)jprocess_from_native(process);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSource(JNIEnv * env, jobject jtask)
{
  const_msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return nullptr;
  }

  auto const* host = MSG_task_get_source(task);
  if (host == nullptr) {
    return nullptr;
  }
  if (not host->extension(JAVA_HOST_LEVEL)) {
    jxbt_throw_jni(env, "MSG_task_get_source() failed");
    return nullptr;
  }

  return (jobject) host->extension(JAVA_HOST_LEVEL);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Task_getFlopsAmount(JNIEnv * env, jobject jtask)
{
  const_msg_task_t ptask = jtask_to_native(jtask, env);

  if (not ptask) {
    jxbt_throw_notbound(env, "task", jtask);
    return -1;
  }
  return (jdouble)MSG_task_get_flops_amount(ptask);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setName(JNIEnv *env, jobject jtask, jobject jname) {
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  const char* name = env->GetStringUTFChars((jstring)jname, nullptr);

  env->SetObjectField(jtask, jtask_field_Task_name, jname);
  MSG_task_set_name(task, name);

  env->ReleaseStringUTFChars((jstring) jname, name);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setPriority(JNIEnv * env, jobject jtask, jdouble priority)
{
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_priority(task, priority);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setFlopsAmount (JNIEnv *env, jobject jtask, jdouble computationAmount)
{
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  MSG_task_set_flops_amount(task, computationAmount);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBytesAmount (JNIEnv *env, jobject jtask, jdouble dataSize)
{
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }
  env->SetDoubleField(jtask, jtask_field_Task_messageSize, dataSize);
  MSG_task_set_bytes_amount(task, dataSize);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_sendBounded(JNIEnv * env,jobject jtask, jstring jalias,
                                                             jdouble jtimeout,jdouble maxrate)
{
  msg_task_t task = jtask_to_native(jtask, env);
  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Add a global ref into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, env->NewGlobalRef(jtask));

  const char* alias = env->GetStringUTFChars(jalias, nullptr);
  msg_error_t res   = MSG_task_send_with_timeout_bounded(task, alias, jtimeout, maxrate);
  env->ReleaseStringUTFChars(jalias, alias);

  if (res != MSG_OK)
    jmsg_throw_status(env, res);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receive(JNIEnv* env, jclass cls, jstring jalias, jdouble jtimeout)
{
  msg_task_t task = nullptr;

  const char* alias = env->GetStringUTFChars(jalias, nullptr);
  msg_error_t rv;
  if (not simgrid::ForcefulKillException::try_n_catch(
          [&rv, &task, &alias, &jtimeout]() { rv = MSG_task_receive_with_timeout(&task, alias, (double)jtimeout); })) {
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", "Process killed");
  }
  env->ReleaseStringUTFChars(jalias, alias);
  if (env->ExceptionOccurred())
    return nullptr;
  if (rv != MSG_OK) {
    jmsg_throw_status(env, rv);
    return nullptr;
  }
  auto jtask_global = (jobject)MSG_task_get_data(task);

  /* Convert the global ref into a local ref so that the JVM can free the stuff */
  jobject jtask_local = env->NewLocalRef(jtask_global);
  env->DeleteGlobalRef(jtask_global);
  MSG_task_set_data(task, nullptr);

  return (jobject) jtask_local;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecv(JNIEnv * env, jclass cls, jstring jmailbox) {
  jclass comm_class = env->FindClass("org/simgrid/msg/Comm");
  if (not comm_class)
    return nullptr;

  //pointer to store the task object pointer.
  auto* task = new msg_task_t(nullptr);
  /* There should be a cache here */

  jobject jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  if (not jcomm) {
    jxbt_throw_jni(env, "Can't create a Comm object.");
    return nullptr;
  }

  const char* mailbox = env->GetStringUTFChars(jmailbox, nullptr);
  msg_comm_t comm     = MSG_task_irecv(task, mailbox);
  env->ReleaseStringUTFChars(jmailbox, mailbox);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(task));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_TRUE);

  return jcomm;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receiveBounded(JNIEnv* env, jclass cls, jstring jalias,
                                                                   jdouble jtimeout, jdouble rate)
{
  msg_task_t task = nullptr;

  const char* alias = env->GetStringUTFChars(jalias, nullptr);
  msg_error_t res   = MSG_task_receive_with_timeout_bounded(&task, alias, jtimeout, rate);
  if (env->ExceptionOccurred())
    return nullptr;
  if (res != MSG_OK) {
    jmsg_throw_status(env, res);
    return nullptr;
  }
  auto jtask_global = (jobject)MSG_task_get_data(task);

  /* Convert the global ref into a local ref so that the JVM can free the stuff */
  jobject jtask_local = env->NewLocalRef(jtask_global);
  env->DeleteGlobalRef(jtask_global);
  MSG_task_set_data(task, nullptr);

  env->ReleaseStringUTFChars(jalias, alias);

  return (jobject) jtask_local;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecvBounded(JNIEnv * env, jclass cls, jstring jmailbox,
                                                                 jdouble rate)
{
  jclass comm_class = env->FindClass("org/simgrid/msg/Comm");
  if (not comm_class)
    return nullptr;

  // pointer to store the task object pointer.
  auto* task = new msg_task_t(nullptr);

  jobject jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  if (not jcomm) {
    jxbt_throw_jni(env, "Can't create a Comm object.");
    return nullptr;
  }

  const char* mailbox = env->GetStringUTFChars(jmailbox, nullptr);
  msg_comm_t comm     = MSG_task_irecv_bounded(task, mailbox, rate);
  env->ReleaseStringUTFChars(jmailbox, mailbox);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(task));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_TRUE);

  return jcomm;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isend(JNIEnv *env, jobject jtask, jstring jmailbox)
{
  msg_comm_t comm;

  jclass comm_class = env->FindClass("org/simgrid/msg/Comm");

  if (not comm_class)
    return nullptr;

  jobject jcomm       = env->NewObject(comm_class, jtask_method_Comm_constructor);
  const char* mailbox = env->GetStringUTFChars(jmailbox, nullptr);

  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    env->ReleaseStringUTFChars(jmailbox, mailbox);
    env->DeleteLocalRef(jcomm);
    jxbt_throw_notbound(env, "task", jtask);
        return nullptr;
  }

  MSG_task_set_data(task, env->NewGlobalRef(jtask));
  comm = MSG_task_isend(task,mailbox);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(nullptr));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_FALSE);

  env->ReleaseStringUTFChars(jmailbox, mailbox);

  return jcomm;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isendBounded(JNIEnv *env, jobject jtask, jstring jmailbox,
                                                                 jdouble maxrate)
{
  msg_task_t task;
  jobject jcomm;
  msg_comm_t comm;
  const char *mailbox;

  jclass comm_class = env->FindClass("org/simgrid/msg/Comm");
  if (not comm_class)
    return nullptr;

  jcomm = env->NewObject(comm_class, jtask_method_Comm_constructor);
  mailbox = env->GetStringUTFChars(jmailbox, nullptr);

  task = jtask_to_native(jtask, env);

  if (not task) {
    env->ReleaseStringUTFChars(jmailbox, mailbox);
    env->DeleteLocalRef(jcomm);
    jxbt_throw_notbound(env, "task", jtask);
        return nullptr;
  }

  MSG_task_set_data(task, env->NewGlobalRef(jtask));
  comm = MSG_task_isend_bounded(task,mailbox,maxrate);

  env->SetLongField(jcomm, jtask_field_Comm_bind, (jlong) (uintptr_t)(comm));
  env->SetLongField(jcomm, jtask_field_Comm_taskBind, (jlong) (uintptr_t)(nullptr));
  env->SetBooleanField(jcomm, jtask_field_Comm_receiving, JNI_FALSE);

  env->ReleaseStringUTFChars(jmailbox, mailbox);

  return jcomm;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeFinalize(JNIEnv * env, jobject jtask)
{
  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    jxbt_throw_notbound(env, "task", jtask);
    return;
    }

    MSG_task_destroy(task);
}

static void msg_task_cancel_on_failed_dsend(void*t) {
  auto task       = (msg_task_t)t;
  JNIEnv* env     = get_current_thread_env();
  if (env) {
    auto jtask_global = (jobject)MSG_task_get_data(task);
    /* Destroy the global ref so that the JVM can free the stuff */
    env->DeleteGlobalRef(jtask_global);
    /* Don't free the C data here, to avoid a race condition with the GC also sometimes doing so.
     * A rare memleak is seen as preferable to a rare "free(): invalid pointer" failure that
     * proves really hard to debug.
     */
  }
  MSG_task_set_data(task, nullptr);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsend(JNIEnv * env, jobject jtask, jstring jalias)
{
  const char* alias = env->GetStringUTFChars(jalias, nullptr);

  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    env->ReleaseStringUTFChars(jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, env->NewGlobalRef(jtask));
  MSG_task_dsend(task, alias, msg_task_cancel_on_failed_dsend);

  env->ReleaseStringUTFChars(jalias, alias);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsendBounded(JNIEnv * env, jobject jtask, jstring jalias,
                                                              jdouble maxrate)
{
  const char* alias = env->GetStringUTFChars(jalias, nullptr);

  msg_task_t task = jtask_to_native(jtask, env);

  if (not task) {
    env->ReleaseStringUTFChars(jalias, alias);
    jxbt_throw_notbound(env, "task", jtask);
    return;
  }

  /* Pass a global ref to the Jtask into the Ctask so that the receiver can use it */
  MSG_task_set_data(task, env->NewGlobalRef(jtask));
  MSG_task_dsend_bounded(task, alias, msg_task_cancel_on_failed_dsend, maxrate);

  env->ReleaseStringUTFChars(jalias, alias);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Task_listen(JNIEnv * env, jclass cls, jstring jalias)
{
  const char* alias = env->GetStringUTFChars(jalias, nullptr);
  int rv = MSG_task_listen(alias);
  env->ReleaseStringUTFChars(jalias, alias);

  return (jboolean) rv;
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFrom(JNIEnv * env, jclass cls, jstring jalias)
{
  const char* alias = env->GetStringUTFChars(jalias, nullptr);
  int rv = MSG_task_listen_from(alias);
  env->ReleaseStringUTFChars(jalias, alias);

  return (jint) rv;
}
