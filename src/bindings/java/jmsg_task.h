/* Functions related to the java task instances.                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JTASK_H
#define MSG_JTASK_H

#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL()

/**
 * This function associated a native task to a java task instance.
 *
 * @param jtask    The java task instance.
 * @param task     The native task to bind.
 * @param env      The environment of the current thread.
 *
 * @exception      If the class Task is not found the function throws the ClassNotFoundException. If the field bind of
 *                 this class is not found the function throws the exception NotSuchFieldException.
 */
void jtask_bind(jobject jtask, msg_task_t task, JNIEnv * env);

/**
 * This function returns a native task from a java task instance.
 *
 * @param jtask   The java task object from which get the native task.
 * @param env     The environment of the current thread.
 *
 * @return        The function returns the native task associated to the java task object.
 *
 * @exception     If the class Task is not found the function throws the ClassNotFoundException. If the field bind of
 *                this class is not found the function throws the exception NotSuchFieldException.
 */
msg_task_t jtask_to_native_task(jobject jtask, JNIEnv * env);

/**
 * This function tests if a java task instance is valid.
 * A java task object is valid if it is bind to a native task.
 *
 * @param jtask    The java task to test the validity.
 * @param env      The environment of the current thread.
 *
 * @return         If the java task is valid the function returns true. Otherwise the function returns false.
 */
jboolean jtask_is_valid(jobject jtask, JNIEnv * env);

/*
 * Class    org_simgrid_msg_Task
 * Method    nativeInit
 * Signature  ();
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeInit(JNIEnv *env, jclass cls);

/*
 * Class    org_simgrid_msg_Task
 * Method    create
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_create (JNIEnv * env, jobject jtask, jstring jname,
                                                         jdouble jcomputeDuration, jdouble jmessageSize);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_nativeFinalize(JNIEnv * env, jobject jtask);

/*
 * Class    org_simgrid_msg_Task
 * Method    parallelCreate
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_parallelCreate (JNIEnv *, jobject, jstring, jobjectArray,
                                                                 jdoubleArray, jdoubleArray);

/*
 * Class    org_simgrid_msg_Task
 * Method    destroy
 * Signature  (Lsimgrid/msg/Task;)V
 */
/* JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_destroy (JNIEnv *, jobject); */

/*
 * Class    org_simgrid_msg_Task
 * Method    cancel
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_cancel (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Task
 * Method    execute
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_execute (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Task
 * Method    setBound
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBound (JNIEnv *, jobject, jdouble);

/*
 * Class    org_simgrid_msg_Task
 * Method    getName
 * Signature  ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_simgrid_msg_Task_getName (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Task
 * Method    getSender
 * Signature  ()Lsimgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSender (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Task
 * Method    getSource
 * Signature  ()Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_getSource (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Task
 * Method    getFlopsAmount
 * Signature  ()D
 */
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Task_getFlopsAmount (JNIEnv *, jobject);

/**
 * Class    org_simgrid_msg_Task
 * Method    setName
 * Signature  (Ljava/lang/string;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setName(JNIEnv *env, jobject jtask, jobject jname);

/*
 * Class    org_simgrid_msg_Task
 * Method    setPriority
 * Signature  (D)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setPriority (JNIEnv *, jobject, jdouble);

/**
 * Class    org_simgrid_msg_Task
 * Method    setComputeDuration
 * Signature  (D)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setFlopsAmount (JNIEnv *env, jobject jtask, jdouble computationAmount);

/**
 * Class    org_simgrid_msg_Task
 * Method    setDataSize
 * Signature  (D)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_setBytesAmount (JNIEnv *env, jobject jtask, jdouble dataSize);

/**
 * Class    org_simgrid_msg_Task
 * Method    send
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_send (JNIEnv *, jobject, jstring, jdouble);

/**
 * Class    org_simgrid_msg_Task
 * Method    sendBounded
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_sendBounded (JNIEnv *, jobject, jstring, jdouble, jdouble);

/**
 * Class    org_simgrid_msg_Task
 * Method    receive
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receive (JNIEnv *, jclass, jstring, jdouble, jobject);

/**
 * Class     org_simgrid_msg_Task
 * Method    irecv
 * Signature  (Ljava/lang/String;)Lorg/simgrid/msg/Comm;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecv(JNIEnv * env, jclass cls, jstring jmailbox);

/**
 * Class     org_simgrid_msg_Task
 * Method    receiveBounded
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_receiveBounded(JNIEnv * env, jclass cls, jstring jalias,
                                                                   jdouble jtimeout, jobject jhost, jdouble rate);

/**
 * Class     org_simgrid_msg_Task
 * Method    irecvBounded
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_irecvBounded(JNIEnv * env, jclass cls, jstring jmailbox,
                                                                 jdouble rate);

/**
 * Class     org_simgrid_msg_Task
 * Method    isend
 * Signature  (Lorg/simgrid/msg/Task;Ljava/lang/String;)Lorg/simgrid/msg/Comm;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isend(JNIEnv *env, jobject jtask, jstring jmailbox);

/**
 * Class     org_simgrid_msg_Task
 * Method    isendBounded
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Task_isendBounded(JNIEnv *env, jobject jtask, jstring jmailbox,
                                                                 jdouble maxrate);

/**
 * Class     org_simgrid_msg_Task
 * Method    dsend
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsend(JNIEnv * env, jobject jtask, jstring jalias);

/**
 * Class     org_simgrid_msg_Task
 * Method    dsendBounded
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Task_dsendBounded(JNIEnv * env, jobject jtask, jstring jalias,
                                                              jdouble maxrate);

/**
 * Class     org_simgrid_msg_Task
 * Method    listen
 */
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Task_listen(JNIEnv *, jclass, jstring);

/**
 * Class     org_simgrid_msg_Task
 * Method    listenFromHost
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFromHost(JNIEnv *, jclass, jstring, jobject);

/**
 * Class     org_simgrid_msg_Task
 * Method    listenFrom
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Task_listenFrom(JNIEnv *, jclass, jstring);

SG_END_DECL()
#endif                          /* !MSG_JTASK_H */
