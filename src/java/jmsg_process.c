/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the implementation of the functions in relation with the java
 * process instance. 
 */

#include "jmsg_process.h"
#include "jmsg.h"
#include "jxbt_utilities.h"

#include "xbt/xbt_context_java.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

jobject jprocess_new_global_ref(jobject jprocess, JNIEnv * env)
{
  return (*env)->NewGlobalRef(env, jprocess);
}

void jprocess_delete_global_ref(jobject jprocess, JNIEnv * env)
{
  (*env)->DeleteGlobalRef(env, jprocess);
}

jboolean jprocess_is_alive(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "isAlive", "()Z");

  if (!id)
    return 0;

  return (*env)->CallBooleanMethod(env, jprocess, id);
}

void jprocess_join(jobject jprocess, JNIEnv * env)
{
  jmethodID id = jxbt_get_smethod(env, "simgrid/msg/Process", "join", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}

void jprocess_exit(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "interrupt", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}

void jprocess_yield(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "switchProcess", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}

void jprocess_lock_mutex(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "lockMutex", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}

void jprocess_unlock_mutex(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "unlockMutex", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}


void jprocess_signal_cond(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "signalCond", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}

void jprocess_wait_cond(jobject jprocess, JNIEnv * env)
{
  jmethodID id =
    jxbt_get_smethod(env, "simgrid/msg/Process", "waitCond", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, jprocess, id);
}


void jprocess_start(jobject jprocess, JNIEnv * env)
{
  jmethodID id = jxbt_get_smethod(env, "simgrid/msg/Process", "start", "()V");

  if (!id)
    return;

  DEBUG2("jprocess_start(jproc=%p,env=%p)", jprocess, env);
  (*env)->CallVoidMethod(env, jprocess, id);
  DEBUG0("jprocess started");
}

m_process_t jprocess_to_native_process(jobject jprocess, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Process", "bind", "J");

  if (!id)
    return NULL;

  return (m_process_t) (long) (*env)->GetLongField(env, jprocess, id);
}

void jprocess_bind(jobject jprocess, m_process_t process, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Process", "bind", "J");

  if (!id)
    return;

  (*env)->SetLongField(env, jprocess, id, (jlong) (long) (process));
}

jlong jprocess_get_id(jobject jprocess, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Process", "id", "J");

  if (!id)
    return 0;

  return (*env)->GetLongField(env, jprocess, id);
}

jstring jprocess_get_name(jobject jprocess, JNIEnv * env)
{
  jfieldID id =
    jxbt_get_sfield(env, "simgrid/msg/Process", "name", "Ljava/lang/String;");
  jobject jname;

  if (!id)
    return NULL;

  jname = (jstring) (*env)->GetObjectField(env, jprocess, id);

  return (*env)->NewGlobalRef(env, jname);

}

jboolean jprocess_is_valid(jobject jprocess, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Process", "bind", "J");

  if (!id)
    return JNI_FALSE;

  return (*env)->GetLongField(env, jprocess, id) ? JNI_TRUE : JNI_FALSE;
}

void jprocess_schedule(xbt_context_t context)
{
  JNIEnv *env;
  jmethodID id;

  env = get_current_thread_env();

  id = jxbt_get_smethod(env, "simgrid/msg/Process", "schedule", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, ((xbt_ctx_java_t) context)->jprocess, id);
}



void jprocess_unschedule(xbt_context_t context)
{
  JNIEnv *env;
  jmethodID id;

  env = get_current_thread_env();


  id = jxbt_get_smethod(env, "simgrid/msg/Process", "unschedule", "()V");

  if (!id)
    return;

  (*env)->CallVoidMethod(env, ((xbt_ctx_java_t) context)->jprocess, id);
}
