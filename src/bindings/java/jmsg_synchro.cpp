/* Java bindings of the Synchronization API.                                */

/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg_synchro.h"
#include "jmsg.hpp"
#include "jxbt_utilities.hpp"
#include "simgrid/Exception.hpp"
#include "xbt/synchro.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

static jfieldID jsynchro_field_Mutex_bind;

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_nativeInit(JNIEnv *env, jclass cls) {
  jsynchro_field_Mutex_bind = jxbt_get_sfield(env, "org/simgrid/msg/Mutex", "bind", "J");
  xbt_assert(jsynchro_field_Mutex_bind, "Native initialization of msg/Mutex failed. Please report that bug");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_init(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex = xbt_mutex_init();

  env->SetLongField(obj, jsynchro_field_Mutex_bind, (jlong)(uintptr_t)(mutex));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_acquire(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex = (xbt_mutex_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Mutex_bind);
  xbt_mutex_acquire(mutex);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_release(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex;

  mutex = (xbt_mutex_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Mutex_bind);
  xbt_mutex_release(mutex);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_nativeFinalize(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex = (xbt_mutex_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Mutex_bind);
  xbt_mutex_destroy(mutex);
}

static jfieldID jsynchro_field_Semaphore_bind;

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_nativeInit(JNIEnv *env, jclass cls) {
  jsynchro_field_Semaphore_bind = jxbt_get_sfield(env, "org/simgrid/msg/Semaphore", "bind", "J");
  xbt_assert(jsynchro_field_Semaphore_bind, "Native initialization of msg/Semaphore failed. Please report that bug");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_init(JNIEnv * env, jobject obj, jint capacity) {
  msg_sem_t sem = MSG_sem_init((int) capacity);

  env->SetLongField(obj, jsynchro_field_Semaphore_bind, (jlong)(uintptr_t)(sem));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_acquire(JNIEnv * env, jobject obj, jdouble timeout) {
  msg_sem_t sem;

  sem             = (msg_sem_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Semaphore_bind);
  msg_error_t res = MSG_sem_acquire_timeout(sem, (double)timeout) == 0 ? MSG_OK : MSG_TIMEOUT;
  if (res != MSG_OK) {
    jmsg_throw_status(env, res);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_release(JNIEnv * env, jobject obj) {
  msg_sem_t sem;

  sem = (msg_sem_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Semaphore_bind);
  MSG_sem_release(sem);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Semaphore_wouldBlock(JNIEnv * env, jobject obj) {
  msg_sem_t sem;

  sem     = (msg_sem_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Semaphore_bind);
  int res = MSG_sem_would_block(sem);
  return (jboolean) res;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_nativeFinalize(JNIEnv * env, jobject obj) {
  msg_sem_t sem;

  sem = (msg_sem_t)(uintptr_t)env->GetLongField(obj, jsynchro_field_Semaphore_bind);
  MSG_sem_destroy(sem);
}
