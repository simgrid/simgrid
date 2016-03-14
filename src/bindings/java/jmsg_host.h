/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JHOST_H
#define MSG_JHOST_H

#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL()

/**
 * This function returns a new java host instance.
 *
 * @param env     The environment of the current thread
 *
 * @return        A new java host object.
 *
 * @exception     If the class Host is not found the function throws the ClassNotFoundException. If the constructor of
 *            this class is not found the function throws the exception NotSuchMethodException.
 */
jobject jhost_new_instance(JNIEnv * env);

/**
 * This function returns a global reference to the  java host instance specified by the parameter jhost.
 *
 * @param jhost   The original java host instance.
 * @param env     The environment of the current thread
 *
 * @return        The global reference to the original java host instance.
 */
jobject jhost_ref(JNIEnv * env, jobject jhost);

/**
 * This function delete a global reference to a java host instance.
 *
 * @param        The global refernce to delete.
 * @param env    The environment of the current thread
 */
void jhost_unref(JNIEnv * env, jobject jhost);

/**
 * This function associated a native host to a java host instance.
 *
 * @param jhost    The java host instance.
 * @param host     The native host to bind.
 * @param env      The environment of the current thread
 *
 * @exception      If the class Host is not found the function throws the ClassNotFoundException. If the field bind of
 *                 this class is not found the function throws the exception NotSuchFieldException.
 */
void jhost_bind(jobject jhost, msg_host_t host, JNIEnv * env);

/**
 * This function returns a native host from a java host instance.
 *
 * @param jhost   The java host object from which get the native host.
 * @param env     The environment of the current thread
 *
 * @return        The function returns the native host associated to the java host object.
 *
 * @exception     If the class Host is not found the function throws the ClassNotFoundException. If the field bind of
 *                this class is not found the function throws the exception NotSuchFieldException.
 */
msg_host_t jhost_get_native(JNIEnv * env, jobject jhost);

/**
 * This function returns the name of a MSG host.
 *
 * @param jhost   A java host object.
 * @param env     The environment of the current thread
 *
 * @return        The name of the host.
 */
const char *jhost_get_name(jobject jhost, JNIEnv * env);

/**
 * This function tests if a java host instance is valid.
 * A java host object is valid if it is bind to a native host.
 *
 * @param jhost    The host to test the validity.
 * @param env      The environment of the current thread
 *
 * @return         If the java host is valid the function returns true. Otherwise the function returns false.
 */
jboolean jhost_is_valid(jobject jhost, JNIEnv * env);

/*
 * Class    org_simgrid_msg_Host
 * Method    nativeInit
 * Signature  ();
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_nativeInit(JNIEnv *env, jclass cls);

/*
 * Class    org_simgrid_msg_Host
 * Method    getByName
 * Signature  (Ljava/lang/String;)Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getByName (JNIEnv *, jclass, jstring);

/**
 * This function start the host if it is off
 *
 * @param jhost    The host to test the validity.
 * @param env      The environment of the current thread
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_on(JNIEnv *env, jobject jhost);

/**
 * This function stop the host if it is on
 *
 * @param jhost    The host to test the validity.
 * @param env      The environment of the current thread
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_off(JNIEnv *env, jobject jhost);

/*
 * Class    org_simgrid_msg_Host
 * Method    currentHost
 * Signature  ()Lsimgrid/msg/Host;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_currentHost (JNIEnv *, jclass);

/*
 * Class    org_simgrid_msg_Host
 * Method    getCount
 * Signature  ()I
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getCount (JNIEnv *, jclass);

/*
 * Class    org_simgrid_msg_Host
 * Method    getSpeed
 * Signature  ()D
 */
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getSpeed (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Host
 * Method    getCoreNumber
 * Signature  ()D
 */
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCoreNumber (JNIEnv *, jobject);

/*
 * Class        org_simgrid_msg_Host
 * Method       getProperty
 * Signature    (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getProperty(JNIEnv *env, jobject jhost, jobject jname);

/*
 * Class        org_simgrid_msg_Host
 * Method       setProperty
 * Signature    (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setProperty(JNIEnv *env, jobject jhost, jobject jname, jobject jvalue);

/*
 * Class    org_simgrid_msg_Host
 * Method    isOn
 * Signature  ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Host_isOn (JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Host
 * Method    getMountedStorage
 * Signature: ()[Lorg/simgrid/msg/Storage;
 */
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getMountedStorage(JNIEnv * env, jobject jhost);

/*
 * Class    org_simgrid_msg_Host
 * Method    getAttachedStorageList
 */
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getAttachedStorage(JNIEnv * env, jobject jhost);

/*
 * Class    org_simgrid_msg_Host
 * Method    getStorageContent
 */
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getStorageContent(JNIEnv * env, jobject jhost);

/**
 * Class org_simgrid_msg_Host
 * Method all
 */
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_all(JNIEnv *, jclass);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setAsyncMailbox(JNIEnv * env, jclass cls_arg, jobject jname);

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getConsumedEnergy (JNIEnv *, jobject);

SG_END_DECL()
#endif                          /*!MSG_JHOST_H */

