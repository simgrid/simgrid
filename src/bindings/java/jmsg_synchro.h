/* Functions related to the java process instances.                         */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JSYNCHRO_H
#define MSG_JSYNCHRO_H

#include <jni.h>
#include <simgrid/msg.h>
#include <simgrid/simix.h>

SG_BEGIN_DECL()

JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_nativeInit(JNIEnv *env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_init(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_acquire(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_release(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Mutex_nativeFinalize(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_nativeInit(JNIEnv *env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_init(JNIEnv * env, jobject obj, jint capacity);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_acquire(JNIEnv * env, jobject obj, jdouble timeout);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_release(JNIEnv * env, jobject obj);
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Semaphore_wouldBlock(JNIEnv * env, jobject obj);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Semaphore_nativeFinalize(JNIEnv * env, jobject obj);

SG_END_DECL()
#endif                          /* !MSG_JPROCESS_H */
