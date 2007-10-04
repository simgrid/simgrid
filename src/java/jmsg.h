/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the declarations of the wrapper functions used to interface
 * the java object with the native functions of the MSG API.
 */

#ifndef MSG4JAVA_H
#define MSG4JAVA_H

#include <jni.h>

JavaVM *
get_java_VM(void);

JNIEnv *
get_current_thread_env(void);

/*
 * Class		simgrid_msg_Msg
 * Method		processCreate
 * Signature	(Lsimgrid/msg/Process;Lsimgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processCreate(JNIEnv* env,jclass cls,jobject jprocess,jobject jhost);

/*
 * Class		simgrid_msg_Msg
 * Method		processSuspend
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_processSuspend(JNIEnv* env, jclass cls, jobject jprocess);

/*
 * Class		simgrid_msg_Msg
 * Method		processResume
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_processResume
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processIsSuspended
 * Signature	(Lsimgrid/msg/Process;)Z
 */
JNIEXPORT jboolean JNICALL Java_simgrid_msg_Msg_processIsSuspended
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processKill
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_processKill
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processGetHost
 * Signature	(Lsimgrid/msg/Process;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_processGetHost
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processFromPID
 * Signature	(I)Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_processFromPID
  (JNIEnv *, jclass, jint);

/*
 * Class		simgrid_msg_Msg
 * Method		processGetPID
 * Signature	(Lsimgrid/msg/Process;)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_processGetPID
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processGetPPID
 * Signature	(Lsimgrid/msg/Process;)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_processGetPPID
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processSelf
 * Signature	()Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_processSelf
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		processSelfPID
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_processSelfPID
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		processSelfPPID
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_processSelfPPID
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		processChangeHost
 * Signature	(Lsimgrid/msg/Process;Lsimgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_processChangeHost
  (JNIEnv *, jclass, jobject, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		processWaitFor
 * Signature	(D)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_processWaitFor
  (JNIEnv *, jclass, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetByName
 * Signature	(Ljava/lang/String;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_hostGetByName
  (JNIEnv *, jclass, jstring);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetName
 * Signature	(Lsimgrid/msg/Host;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_simgrid_msg_Msg_hostGetName
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetNumber
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_hostGetNumber
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		hostSelf
 * Signature	()Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_hostSelf
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		hostGetSpeed
 * Signature	(Lsimgrid/msg/Host;)D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_Msg_hostGetSpeed
  (JNIEnv *, jclass, jobject);

JNIEXPORT jint JNICALL 
Java_simgrid_msg_Msg_hostGetLoad(JNIEnv* env, jclass cls, jobject jhost);

/*
 * Class		simgrid_msg_Msg
 * Method		hostIsAvail
 * Signature	(Lsimgrid/msg/Host;)Z
 */
JNIEXPORT jboolean JNICALL Java_simgrid_msg_Msg_hostIsAvail
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		taskCreate
 * Signature	(Lsimgrid/msg/Task;Ljava/lang/String;DD)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_taskCreate
  (JNIEnv *, jclass, jobject, jstring, jdouble, jdouble);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallel_taskCreate(JNIEnv*, jclass, jobject, jstring, jobjectArray,jdoubleArray, jdoubleArray);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetSender
 * Signature	(Lsimgrid/msg/Task;)Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_taskGetSender
  (JNIEnv *, jclass, jobject);

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetSender(JNIEnv* env , jclass cls , jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetSource
 * Signature	(Lsimgrid/msg/Task;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_taskGetSource
  (JNIEnv *, jclass, jobject);

JNIEXPORT jobject JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetSource(JNIEnv* env , jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetName
 * Signature	(Lsimgrid/msg/Task;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_simgrid_msg_Msg_taskGetName
  (JNIEnv *, jclass, jobject);

JNIEXPORT jstring JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetName(JNIEnv* env, jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskCancel
 * Signature	(Lsimgrid/msg/Task;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_taskCancel
  (JNIEnv *, jclass, jobject);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskCancel(JNIEnv* env, jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetComputeDuration
 * Signature	(Lsimgrid/msg/Task;)D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_Msg_taskGetComputeDuration
  (JNIEnv *, jclass, jobject);

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_parallelTaskGetComputeDuration(JNIEnv* env, jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskGetRemainingDuration
 * Signature	(Lsimgrid/msg/Task;)D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_Msg_taskGetRemainingDuration
  (JNIEnv *, jclass, jobject);

JNIEXPORT jdouble JNICALL 
Java_simgrid_msg_Msg_paralleTaskGetRemainingDuration(JNIEnv* env, jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskSetPriority
 * Signature	(Lsimgrid/msg/Task;D)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_taskSetPriority
  (JNIEnv *, jclass, jobject, jdouble);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskSetPriority(JNIEnv* env, jclass cls, jobject jparallel_task, jdouble priority);

/*
 * Class		simgrid_msg_Msg
 * Method		taskDestroy
 * Signature	(Lsimgrid/msg/Task;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_taskDestroy
  (JNIEnv *, jclass, jobject);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskDestroy(JNIEnv* env, jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		taskExecute
 * Signature	(Lsimgrid/msg/Task;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_taskExecute
  (JNIEnv *, jclass, jobject);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_parallelTaskExecute(JNIEnv* env, jclass cls, jobject jparallel_task);

/*
 * Class		simgrid_msg_Msg
 * Method		channelGet
 * Signature	(Lsimgrid/msg/Channel;)Lsimgrid/msg/Task;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_channelGet
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		channelGetWithTimeout
 * Signature	(Lsimgrid/msg/Channel;D)Lsimgrid/msg/Task;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_channelGetWithTimeout
  (JNIEnv *, jclass, jobject, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		channelGetFromHost
 * Signature	(Lsimgrid/msg/Channel;Lsimgrid/msg/Host;)Lsimgrid/msg/Task;
 */
JNIEXPORT jobject JNICALL Java_simgrid_msg_Msg_channelGetFromHost
  (JNIEnv *, jclass, jobject, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		channelHasPendingCommunication
 * Signature	(Lsimgrid/msg/Channel;)Z
 */
JNIEXPORT jboolean JNICALL Java_simgrid_msg_Msg_channelHasPendingCommunication
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		channelGetCommunicatingProcess
 * Signature	(Lsimgrid/msg/Channel;)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_channelGetCommunicatingProcess
  (JNIEnv *, jclass, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		channelGetHostWaitingTasks
 * Signature	(Lsimgrid/msg/Channel;Lsimgrid/msg/Host;)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_channelGetHostWaitingTasks
  (JNIEnv *, jclass, jobject, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		channelPut
 * Signature	(Lsimgrid/msg/Channel;Lsimgrid/msg/Task;Lsimgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_channelPut
  (JNIEnv *, jclass, jobject, jobject, jobject);

/*
 * Class		simgrid_msg_Msg
 * Method		channelPutWithTimeout
 * Signature	(Lsimgrid/msg/Channel;Lsimgrid/msg/Task;Lsimgrid/msg/Host;D)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_channelPutWithTimeout
  (JNIEnv *, jclass, jobject, jobject, jobject, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		channelPutBounded
 * Signature	(Lsimgrid/msg/Channel;Lsimgrid/msg/Task;Lsimgrid/msg/Host;D)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_channelPutBounded
  (JNIEnv *, jclass, jobject, jobject, jobject, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		channelWait
 * Signature	(Lsimgrid/msg/Channel;D)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_channelWait
  (JNIEnv *, jclass, jobject, jdouble);

/*
 * Class		simgrid_msg_Msg
 * Method		channelSetNumber
 * Signature	(I)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_channelSetNumber
  (JNIEnv *, jclass, jint);

/*
 * Class		simgrid_msg_Msg
 * Method		channelGetNumber
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_channelGetNumber
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		getErrCode
 * Signature	()I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_getErrCode
  (JNIEnv *, jclass);

/*
 * Class		simgrid_msg_Msg
 * Method		getClock
 * Signature	()D
 */
JNIEXPORT jdouble JNICALL Java_simgrid_msg_Msg_getClock
  (JNIEnv *, jclass);


JNIEXPORT void JNICALL Java_simgrid_msg_Msg_init
  (JNIEnv *, jclass, jobjectArray);


JNIEXPORT void JNICALL
JNICALL Java_simgrid_msg_Msg_run(JNIEnv* env, jclass cls);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_init(JNIEnv* env, jclass cls, jobjectArray jargs);

/*
 * Class		simgrid_msg_Msg
 * Method		processKillAll
 * Signature	(I)I
 */
JNIEXPORT jint JNICALL Java_simgrid_msg_Msg_processKillAll
  (JNIEnv *, jclass, jint);

/*
 * Class		simgrid_msg_Msg
 * Method		processExit
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_processExit
  (JNIEnv *, jclass, jobject);


JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_pajeOutput(JNIEnv* env, jclass cls, jstring jpajeFile);

/*
 * Class		simgrid_msg_Msg
 * Method		waitSignal
 * Signature	(Lsimgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_simgrid_msg_Msg_waitSignal
  (JNIEnv *, jclass, jobject);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_info(JNIEnv * , jclass , jstring );

JNIEXPORT jobjectArray JNICALL
Java_simgrid_msg_Msg_allHosts(JNIEnv * , jclass );

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_createEnvironment(JNIEnv* env, jclass cls,jstring jplatformFile);

JNIEXPORT void JNICALL 
Java_simgrid_msg_Msg_platformLoad(JNIEnv* env, jclass cls, jobject jplatform);

#endif /* !MSG4JAVA_H */ 
