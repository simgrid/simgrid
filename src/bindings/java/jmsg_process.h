/* Functions related to the java process instances.                         */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JPROCESS_H
#define MSG_JPROCESS_H

#include <jni.h>
#include <simgrid/msg.h>
#include <simgrid/simix.h>

SG_BEGIN_DECL();

//Cached java fields
extern jfieldID jprocess_field_Process_bind;
extern jfieldID jprocess_field_Process_host;
extern jfieldID jprocess_field_Process_killTime;
extern jfieldID jprocess_field_Process_id;
extern jfieldID jprocess_field_Process_name;
extern jfieldID jprocess_field_Process_pid;
extern jfieldID jprocess_field_Process_ppid;

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_exit(JNIEnv *env, jobject);

jobject native_to_java_process(msg_process_t process);

/**
 * This function returns a global reference to the  java process instance specified by the parameter jprocess.
 *
 * @param jprocess  The original java process instance.
 * @param env       The env of the current thread
 *
 * @return          The global reference to the original java process instance.
 */
jobject jprocess_new_global_ref(jobject jprocess, JNIEnv * env);

/**
 * This function delete a global reference to a java process instance.
 * If the java process is alive the function joins it and stops it before.
 *
 * @param        The global refernce to delete.
 * @param env    The env of the current thread
 *
 * @see          jprocess_join()
 * @see          jprocess_exit()
 */
void jprocess_delete_global_ref(jobject jprocess, JNIEnv * env);

/**
 * This function waits for a java process to terminate.
 *
 * @param jprocess  The java process ot wait for.
 * @param env       The env of the current thread
 *
 * @exception       If the class Process is not found the function throws the ClassNotFoundException. If the method
                    join() of this class is not found the function throws the exception NotSuchMethodException.
 */
void jprocess_join(jobject jprocess, JNIEnv * env);

/**
 * This function associated a native process to a java process instance.
 *
 * @param jprocess   The java process instance.
 * @param process    The native process to bind.
 * @param env        The env of the current thread
 *
 * @exception        If the class Process is not found the function throws the ClassNotFoundException. If the field
 *                   bind of this class is not found the function throws the exception NotSuchFieldException.
 */
void jprocess_bind(jobject jprocess, msg_process_t process, JNIEnv * env);

/**
 * This function returns a native process from a java process instance.
 *
 * @param jprocess   The java process object from which get the native process.
 * @param env        The env of the current thread
 *
 * @return           The function returns the native process associated to the java process object.
 *
 * @exception        If the class Process is not found the function throws the ClassNotFoundException. If the field
 *                   bind of this class is not found the function throws the exception NotSuchFieldException.
 */
msg_process_t jprocess_to_native_process(jobject jprocess, JNIEnv * env);

/**
 * This function gets the id of the specified java process.
 *
 * @param jprocess   The java process to get the id.
 * @param env        The env of the current thread
 *
 * @exception        If the class Process is not found the function throws the ClassNotFoundException. If the field id
 *                   of this class is not found the function throws the exception NotSuchFieldException.
 *
 * @return        The id of the specified java process.
 */
jlong jprocess_get_id(jobject jprocess, JNIEnv * env);

/**
 * This function tests if a java process instance is valid.
 * A java process object is valid if it is bind to a native process.
 *
 * @param jprocess   The java process to test the validity.
 * @param env        The env of the current thread
 *
 * @return           If the java process is valid the function returns true. Otherwise the function returns false.
 */
jboolean jprocess_is_valid(jobject jprocess, JNIEnv * env);

/**
 * This function gets the name of the specified java process.
 *
 * @param jprocess  The java process to get the name.
 * @param env       The env of the current thread
 *
 * @exception       If the class Process is not found the function throws the ClassNotFoundException. If the field name
 *                  of this class is not found the function throws the exception NotSuchFieldException.
 *
 * @return        The name of the specified java process.
 */
jstring jprocess_get_name(jobject jprocess, JNIEnv * env);

/*
 * Class    org_simgrid_msg_Process
 * Method    nativeInit
 * Signature  ();
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_nativeInit(JNIEnv *env, jclass cls);

/*
 * Class    org_simgrid_msg_Process
 * Method    create
 * Signature  (Lorg/simgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_create(JNIEnv * env, jobject jprocess_arg, jobject jhostname);

/*
 * Class    org_simgrid_msg_Process
 * Method    killAll
 * Signature  (I)I
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Process_killAll (JNIEnv *, jclass, jint);

/*
 * Class    org_simgrid_msg_Process
 * Method    fromPID
 * Signature  (I)Lorg/simgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_fromPID (JNIEnv *, jclass, jint);

/*
 * Class        org_simgrid_msg_Process
 * Method       getProperty
 * Signature    (D)V
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_getProperty(JNIEnv *env, jobject jprocess, jobject jname);

/*
 * Class    org_simgrid_msg_Process
 * Method    currentProcess
 * Signature  ()Lorg/simgrid/msg/Process;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_getCurrentProcess (JNIEnv *, jclass);

/*
 * Class    org_simgrid_msg_Process
 * Method    suspend
 * Signature  (Lorg/simgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_suspend(JNIEnv * env, jobject jprocess);

/*
 * Class    org_simgrid_msg_Process
 * Method    resume
 * Signature  (Lorg/simgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_resume (JNIEnv *, jobject);

/*
 * Class        org_simgrid_msg_Process
 * Method       setAutoRestart
 * Signature    (Lorg/simgrid/msg/Process;Z)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_setAutoRestart (JNIEnv *, jobject, jboolean);

/*
 * Class        org_simgrid_msg_Process
 * Method       restart
 * Signature    (Lorg/simgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_restart (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Process
 * Method    isSuspended
 * Signature  (Lorg/simgrid/msg/Process;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Process_isSuspended (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Process
 * Method    sleep
 * Signature  (DI)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_sleep (JNIEnv *, jclass, jlong, jint);

/*
 * Class    org_simgrid_msg_Process
 * Method    waitFor
 * Signature  (D)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_waitFor (JNIEnv *, jobject, jdouble);

/*
 * Class    org_simgrid_msg_Process
 * Method    kill
 * Signature  (Lorg/simgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_kill (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Process
 * Method    migrate
 * Signature  (Lorg/simgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_migrate (JNIEnv *, jobject, jobject);

/*
 * Class    org_simgrid_msg_Process
 * Method    setKillTime
 * Signature  (D)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_setKillTime (JNIEnv *, jobject, jdouble);

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Process_getCount(JNIEnv * env, jclass cls);

SG_END_DECL();
#endif                          /* !MSG_JPROCESS_H */
