/* Functions exporting the simgrid synchronization mechanisms to the Java world */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg.h"
#include "xbt/synchro_core.h"
#include "jmsg_synchro.h"
#include "jxbt_utilities.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jfieldID jsyncro_field_Mutex_bind;

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_nativeInit(JNIEnv *env, jclass cls) {
  jsyncro_field_Mutex_bind = jxbt_get_sfield(env, "org/simgrid/msg/Mutex", "bind", "J");
  if (!jsyncro_field_Mutex_bind) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_init(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex = xbt_mutex_init();

  env->SetLongField(obj, jsyncro_field_Mutex_bind, (jlong) (uintptr_t) (mutex));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_acquire(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex;

  mutex = (xbt_mutex_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Mutex_bind);
  xbt_ex_t e;
  TRY {
    xbt_mutex_acquire(mutex);
  }
  CATCH(e) {
    xbt_ex_free(e);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_release(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex;

  mutex = (xbt_mutex_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Mutex_bind);
  xbt_mutex_release(mutex);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_nativeFinalize(JNIEnv * env, jobject obj) {
  xbt_mutex_t mutex;

  mutex = (xbt_mutex_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Mutex_bind);
  xbt_mutex_destroy(mutex);
}

static jfieldID jsyncro_field_Semaphore_bind;

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_nativeInit(JNIEnv *env, jclass cls) {
  jsyncro_field_Semaphore_bind = jxbt_get_sfield(env, "org/simgrid/msg/Semaphore", "bind", "J");
  if (!jsyncro_field_Semaphore_bind) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Semaphore Java class. You should report this bug."));
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_init(JNIEnv * env, jobject obj, jint capacity) {
  msg_sem_t sem = MSG_sem_init((int) capacity);

  env->SetLongField(obj, jsyncro_field_Semaphore_bind, (jlong) (uintptr_t) (sem));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_acquire(JNIEnv * env, jobject obj, jdouble timeout) {
  msg_sem_t sem;

  sem = (msg_sem_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Semaphore_bind);
  msg_error_t res = MSG_sem_acquire_timeout(sem, (double) timeout);
  if (res != MSG_OK) {
    jmsg_throw_status(env, res);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_release(JNIEnv * env, jobject obj) {
  msg_sem_t sem;

  sem = (msg_sem_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Semaphore_bind);
  MSG_sem_release(sem);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Semaphore_wouldBlock(JNIEnv * env, jobject obj) {
  msg_sem_t sem;

  sem = (msg_sem_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Semaphore_bind);
  int res = MSG_sem_would_block(sem);
  return (jboolean) res;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_nativeFinalize(JNIEnv * env, jobject obj) {
  msg_sem_t sem;

  sem = (msg_sem_t) (uintptr_t) env->GetLongField(obj, jsyncro_field_Semaphore_bind);
  MSG_sem_destroy(sem);
}
