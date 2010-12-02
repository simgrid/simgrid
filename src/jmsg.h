/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG4JAVA_H
#define MSG4JAVA_H

#include <jni.h>

JavaVM *get_java_VM(void);

JNIEnv *get_current_thread_env(void);

/*
 * Class		simgrid_msg_Msg
 * Method		processCreate
 * Signature	(Lsimgrid/msg/Process;Lsimgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processCreate(JNIEnv * env, jclass cls,
                                         jobject jprocess, jobject jhost);



/*
 * Class		simgrid_msg_Msg
 * Method		processSuspend
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_processSuspend(JNIEnv * env, jclass cls,
                                          jobject jprocess);

/*
 * Class		simgrid_msg_Msg
 * Method		processResume
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_processResume
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processIsSuspended
 * Signature	(Lsimgrid/msg/Process;)Z
 */
JNIEXPORT jboolean JNICALL Java_simgrid_msg_MsgNative_processIsSuspended
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processKill
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_processKill
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processGetHost
 * Signature	(Lsimgrid/msg/Process;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_processGetHost
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processFromPID
 * Signature	(I)Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_processFromPID
    (JNIEnv *, jclass, jint);

/*
 * Class		simgrid_msg_Msg
 * Method		processGetPID
 * Signature	(Lsimgrid/msg/Process;)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_MsgNative_processGetPID
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processGetPPID
 * Signature	(Lsimgrid/msg/Process;)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_MsgNative_processGetPPID
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processSelf
 * Signature	()Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_processSelf
    (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		processSelfPID
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_MsgNative_processSelfPID
    (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		processSelfPPID
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_MsgNative_processSelfPPID
    (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		processChangeHost
 * Signature	(Lsimgrid/msg/Process;Lsimgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_processChangeHost
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processWaitFor
 * Signature	(D)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_processWaitFor
    (JNIEnv *, jclass, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetByName
 * Signature	(Ljava/lang/String;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_hostGetByName
    (JNIEnv *, jclass, jstring);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetName
 * Signature	(Lsimgrid/msg/Host;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_simgrid_msg_MsgNative_hostGetName
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetNumber
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_MsgNative_hostGetNumber
    (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		hostSelf
 * Signature	()Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_hostSelf
    (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetSpeed
 * Signature	(Lsimgrid/msg/Host;)D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_MsgNative_hostGetSpeed
    (JNIEnv *, jclass, jobject);

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_hostGetLoad(JNIEnv * env, jclass cls,
                                       jobject jhost);

/*
 * Class		simgrid_msg_Msg
 * Method		hostIsAvail
 * Signature	(Lsimgrid/msg/Host;)Z
 */
JNIEXPORT jboolean JNICALL Java_simgrid_msg_MsgNative_hostIsAvail
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskCreate
 * Signature	(Lsimgrid/msg/Task;Ljava/lang/String;DD)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_taskCreate
    (JNIEnv *, jclass, jobject, jstring, jdouble, jdouble);

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_parallel_taskCreate(JNIEnv *, jclass, jobject,
                                               jstring, jobjectArray,
                                               jdoubleArray, jdoubleArray);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetSender
 * Signature	(Lsimgrid/msg/Task;)Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_taskGetSender
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetSource
 * Signature	(Lsimgrid/msg/Task;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_MsgNative_taskGetSource
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetName
 * Signature	(Lsimgrid/msg/Task;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_simgrid_msg_MsgNative_taskGetName
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskCancel
 * Signature	(Lsimgrid/msg/Task;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_taskCancel
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetComputeDuration
 * Signature	(Lsimgrid/msg/Task;)D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_MsgNative_taskGetComputeDuration
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetRemainingDuration
 * Signature	(Lsimgrid/msg/Task;)D
 */
JNIEXPORT jdouble JNICALL
Java_simgrid_msg_MsgNative_taskGetRemainingDuration(JNIEnv *, jclass,
                                                    jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskSetPriority
 * Signature	(Lsimgrid/msg/Task;D)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_taskSetPriority
    (JNIEnv *, jclass, jobject, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		taskDestroy
 * Signature	(Lsimgrid/msg/Task;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_taskDestroy
    (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskExecute
 * Signature	(Lsimgrid/msg/Task;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_taskExecute
    (JNIEnv *, jclass, jobject);

JNIEXPORT jobject JNICALL
    Java_simgrid_msg_MsgNative_taskReceive
    (JNIEnv *, jclass, jstring, jdouble, jobject);

JNIEXPORT void JNICALL
    Java_simgrid_msg_MsgNative_taskSend
    (JNIEnv *, jclass, jstring, jobject, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		getErrCode
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_getErrCode(JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		getClock
 * Signature	()D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_Msg_getClock(JNIEnv *, jclass);

JNIEXPORT void JNICALL
    JNICALL Java_simgrid_msg_Msg_run(JNIEnv * env, jclass cls);

JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_init(JNIEnv * env, jclass cls, jobjectArray jargs);

/*
 * Class		simgrid_msg_Msg
 * Method		processKillAll
 * Signature	(I)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_MsgNative_processKillAll
    (JNIEnv *, jclass, jint);

/*
 * Class		simgrid_msg_Msg
 * Method		processExit
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_MsgNative_processExit
    (JNIEnv *, jclass, jobject);

JNIEXPORT void JNICALL Java_simgrid_msg_Msg_info(JNIEnv *, jclass,
                                                 jstring);

JNIEXPORT jobjectArray JNICALL
Java_simgrid_msg_MsgNative_allHosts(JNIEnv *, jclass);

JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_createEnvironment(JNIEnv * env, jclass cls,
                                       jstring jplatformFile);

JNIEXPORT void JNICALL
Java_simgrid_msg_MsgNative_taskSendBounded(JNIEnv *, jclass, jstring,
                                           jobject, jdouble);

JNIEXPORT jboolean JNICALL
Java_simgrid_msg_MsgNative_taskListen(JNIEnv *, jclass, jstring);

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_taskListenFromHost(JNIEnv *, jclass, jstring,
                                              jobject);

JNIEXPORT jint JNICALL
Java_simgrid_msg_MsgNative_taskListenFrom(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL
Java_simgrid_msg_Msg_deployApplication(JNIEnv * env, jclass cls,
                                       jstring jdeploymentFile);

#endif                          /* !MSG4JAVA_H */
