/* Functions related to the java comm instances 																*/

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.                   */

#ifndef MSG_JCOMM_H
#define MSG_JCOMM_H
#include <jni.h>

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_unbind(JNIEnv *env, jobject jcomm);

JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_Comm_test(JNIEnv *env, jobject jcomm);
#endif /* MSG_JCOMM_H */
