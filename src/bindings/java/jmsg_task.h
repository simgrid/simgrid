/* Functions related to the java task instances.                            */

/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JTASK_H
#define MSG_JTASK_H

#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL

/** Binds a native instance to a java instance. */
void jtask_bind(jobject jtask, msg_task_t task, JNIEnv * env);

/** Extract the native instance from the java one */
msg_task_t jtask_to_native(jobject jtask, JNIEnv* env);

/** Initialize the native world, called from the Java world at startup */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeInit(JNIEnv* env, jclass cls);

/* Implement the Java API */

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_create(JNIEnv* env, jobject jtask, jstring jname,
                                                        jdouble jcomputeDuration, jdouble jmessageSize);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeFinalize(JNIEnv* env, jobject jtask);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_parallelCreate(JNIEnv* env, jobject jtask,  jstring jname,
                                                                jobjectArray jhosts, jdoubleArray jcomputeDurations_arg,
                                                                jdoubleArray jmessageSizes);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_cancel(JNIEnv* env, jobject jtask);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_execute(JNIEnv* env, jobject jtask);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBound(JNIEnv* env, jobject jtask, jdouble bound);
JNIEXPORT jstring JNICALL Java_org_simgrid_msg_Task_getName(JNIEnv* env, jobject jtask);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setName(JNIEnv* env, jobject jtask, jobject jname);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSender(JNIEnv* env, jobject jtask);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSource(JNIEnv* env, jobject jtask);
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Task_getFlopsAmount(JNIEnv* env, jobject jtask);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setPriority(JNIEnv* env, jobject jtask, jdouble prioriy);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setFlopsAmount(JNIEnv* env, jobject jtask, jdouble computationAmount);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBytesAmount(JNIEnv* env, jobject jtask, jdouble dataSize);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_sendBounded(JNIEnv* env, jobject jtask, jstring jalias,
                                                             jdouble  jtimeout, jdouble maxrate);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receive(JNIEnv* env, jclass cls, jstring jalias, jdouble jtimeout);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecv(JNIEnv* env, jclass cls, jstring jmailbox);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receiveBounded(JNIEnv* env, jclass cls, jstring jalias,
                                                                   jdouble jtimeout, jdouble rate);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecvBounded(JNIEnv* env, jclass cls, jstring jmailbox,
                                                                 jdouble rate);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isend(JNIEnv* env, jobject jtask, jstring jmailbox);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isendBounded(JNIEnv* env, jobject jtask, jstring jmailbox,
                                                                 jdouble maxrate);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsend(JNIEnv* env, jobject jtask, jstring jalias);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsendBounded(JNIEnv* env, jobject jtask, jstring jalias,
                                                              jdouble maxrate);
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Task_listen(JNIEnv* env, jclass cls, jstring jalias);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFromHost(JNIEnv* env, jclass cls, jstring jalias, jobject jhost);
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFrom(JNIEnv* env, jclass cls, jstring jalias);

SG_END_DECL
#endif
