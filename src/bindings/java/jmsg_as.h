/* Functions related to the java As instances.                            */

/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef MSG_JAS_H
#define MSG_JAS_H
#include <jni.h>
#include "msg/msg.h"

/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"
#include "jmsg.h"
#include "jmsg_host.h"

jobject jas_new_instance(JNIEnv * env);
jobject jas_ref(JNIEnv * env, jobject jas);
void jas_unref(JNIEnv * env, jobject jas);
void jas_bind(jobject jas, msg_as_t as, JNIEnv * env);
msg_as_t jas_get_native(JNIEnv * env, jobject jas);

JNIEXPORT void JNICALL
Java_org_simgrid_msg_As_nativeInit(JNIEnv *env, jclass cls);

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_As_getName(JNIEnv * env, jobject jas);

JNIEXPORT jobjectArray JNICALL
Java_org_simgrid_msg_As_getSons(JNIEnv * env, jobject jas);

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_As_getProperty(JNIEnv *env, jobject jhost, jobject jname);


#endif                          /*!MSG_JAS_H */
