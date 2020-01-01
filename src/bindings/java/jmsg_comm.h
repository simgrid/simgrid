/* Java bindings to the Comm API                                            */

/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JCOMM_H
#define MSG_JCOMM_H

#include "simgrid/msg.h"
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

/** This function binds the task associated with the communication to the java communication object. */
void jcomm_bind_task(JNIEnv *env, jobject jcomm);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Comm_nativeInit(JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Comm_nativeFinalize(JNIEnv *env, jobject jcomm);

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Comm_test(JNIEnv *env, jobject jcomm);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Comm_waitCompletion(JNIEnv *env, jobject jcomm, jdouble timeout);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Comm_waitAll(JNIEnv *env, jclass cls, jobjectArray jcomms, jdouble timeout);
JNIEXPORT int JNICALL Java_org_simgrid_msg_Comm_waitAny(JNIEnv *env, jclass cls, jobjectArray jcomms);

SG_END_DECL
#endif
