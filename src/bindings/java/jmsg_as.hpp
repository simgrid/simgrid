/* Functions related to the java As instances.                              */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JMSG_AS_HPP
#define JMSG_AS_HPP
#include "simgrid/msg.h"
#include <jni.h>

extern "C" {

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

jobject jnetzone_new_instance(JNIEnv* env);
jobject jnetzone_ref(JNIEnv* env, jobject jnetzone);
void jnetzone_unref(JNIEnv* env, jobject jnetzone);
void jnetzone_bind(jobject jas, msg_netzone_t as, JNIEnv* env);
simgrid::s4u::NetZone* jnetzone_get_native(JNIEnv* env, jobject jnetzone);

JNIEXPORT void JNICALL Java_org_simgrid_msg_As_nativeInit(JNIEnv* env, jclass cls);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getName(JNIEnv* env, jobject jas);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getSons(JNIEnv* env, jobject jnetzone);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getProperty(JNIEnv* env, jobject jhost, jobject jname);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getHosts(JNIEnv* env, jobject jnetzone);
}
#endif
