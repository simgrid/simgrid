/* Functions related to the java As instances.                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JAS_H
#define MSG_JAS_H
#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL()

jobject jas_new_instance(JNIEnv * env);
jobject jas_ref(JNIEnv * env, jobject jas);
void jas_unref(JNIEnv * env, jobject jas);
void jas_bind(jobject jas, msg_as_t as, JNIEnv * env);
msg_as_t jas_get_native(JNIEnv * env, jobject jas);

JNIEXPORT void JNICALL Java_org_simgrid_msg_As_nativeInit(JNIEnv *env, jclass cls);

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getName(JNIEnv * env, jobject jas);

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getSons(JNIEnv * env, jobject jas);

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getProperty(JNIEnv *env, jobject jhost, jobject jname);

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getHosts(JNIEnv * env, jobject jas);

SG_END_DECL()
#endif                          /*!MSG_JAS_H */
