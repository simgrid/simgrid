/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JHOST_H
#define MSG_JHOST_H

#include "simgrid/msg.h"
#include "simgrid/plugins/file_system.h"
#include <jni.h>

SG_BEGIN_DECL

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

/** Returns a new java instance of an host. */
jobject jhost_new_instance(JNIEnv * env);

/** Take a ref onto the java instance (to prevent its collection) */
jobject jhost_ref(JNIEnv * env, jobject jhost);

/** Release a ref onto the java instance */
void jhost_unref(JNIEnv * env, jobject jhost);

/** Binds a native instance to a java instance. */
void jhost_bind(jobject jhost, msg_host_t host, JNIEnv * env);

/** Extracts the native instance associated to a java instance. */
msg_host_t jhost_get_native(JNIEnv * env, jobject jhost);

/** Initialize the native world, called from the Java world at startup */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_nativeInit(JNIEnv *env, jclass cls);

/* Implement the Java API */

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getByName(JNIEnv* env, jclass cls, jstring jname);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_currentHost(JNIEnv* env, jclass cls);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getCount (JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_on(JNIEnv* env, jobject jhost);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_off(JNIEnv* env, jobject jhost);
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Host_isOn(JNIEnv* env, jobject jhost);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getSpeed (JNIEnv *env, jobject jhost);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCoreNumber (JNIEnv *env, jobject jhost);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getProperty(JNIEnv *env, jobject jhost, jobject jname);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setProperty(JNIEnv *env, jobject jhost, jobject jname, jobject jvalue);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getMountedStorage(JNIEnv * env, jobject jhost);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getAttachedStorage(JNIEnv * env, jobject jhost);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getStorageContent(JNIEnv * env, jobject jhost);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_all(JNIEnv *env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setAsyncMailbox(JNIEnv * env, jclass cls_arg, jobject jname);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_updateAllEnergyConsumptions(JNIEnv* env, jclass cls);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getConsumedEnergy (JNIEnv *env, jobject jhost);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setPstate(JNIEnv* env, jobject jhost, jint pstate);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getPstate(JNIEnv* env, jobject jhost);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getPstatesCount(JNIEnv* env, jobject jhost);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCurrentPowerPeak(JNIEnv* env, jobject jhost);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getPowerPeakAt(JNIEnv* env, jobject jhost, jint pstate);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getLoad(JNIEnv* env, jobject jhost);

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCurrentLoad(JNIEnv *env, jobject jhost);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getAvgLoad(JNIEnv *env, jobject jhost);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getComputedFlops (JNIEnv *env, jobject jhost);
SG_END_DECL
#endif
