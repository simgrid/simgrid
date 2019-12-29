/* Functions related to the java process instances.                         */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JPROCESS_H
#define MSG_JPROCESS_H

#include "simgrid/msg.h"
#include <jni.h>

SG_BEGIN_DECL;

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

// Cached java fields accessed by the rest of the code
extern jfieldID jprocess_field_Process_pid;
extern jfieldID jprocess_field_Process_ppid;

/** Take a ref onto the java instance (to prevent its collection) */
jobject jprocess_ref(jobject jprocess, JNIEnv* env);

/** Release a ref onto the java instance */
void jprocess_unref(jobject jprocess, JNIEnv* env);

/** Binds a native instance to a java instance. */
void jprocess_bind(jobject jprocess, const_sg_actor_t process, JNIEnv* env);

/** Extract the java instance from the native one */
jobject jprocess_from_native(const_sg_actor_t process);

/** Extract the native instance from the java one */
msg_process_t jprocess_to_native(jobject jprocess, JNIEnv* env);

/** Initialize the native world, called from the Java world at startup */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_nativeInit(JNIEnv *env, jclass cls);

/* Implement the Java API */

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_create(JNIEnv* env, jobject jprocess_arg, jobject jhostname);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_daemonize(JNIEnv* env, jobject jprocess);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_killAll(JNIEnv* env, jclass cls);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_fromPID(JNIEnv* env, jclass cls, jint pid);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Process_nativeGetPID(JNIEnv* env, jobject jprocess);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_getProperty(JNIEnv* env, jobject jprocess, jobject jname);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_getCurrentProcess(JNIEnv* env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_suspend(JNIEnv* env, jobject jprocess);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_resume(JNIEnv* env, jobject jprocess);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_setAutoRestart(JNIEnv* env, jobject jprocess, jboolean jauto_restart);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_restart(JNIEnv* env, jobject jprocess);
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Process_isSuspended(JNIEnv* env, jobject jprocess);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_sleep(JNIEnv* env, jclass cls, jlong jmillis, jint jnanos);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_waitFor(JNIEnv* env, jobject jprocess, jdouble jseconds);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_kill(JNIEnv* env, jobject jprocess);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_migrate(JNIEnv* env, jobject jprocess, jobject jhost);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_yield(JNIEnv* env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_setKillTime(JNIEnv* env, jobject jprocess, jdouble jkilltime);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Process_getCount(JNIEnv * env, jclass cls);

SG_END_DECL;
#endif
