/* Functions related to the java storage API.                            */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JSTORAGE_H
#define MSG_JSTORAGE_H
#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL()

/**
 * This function returns a new java storage instance.
 *
 * @param env   The environment of the current thread
 *
 * @return      A new java storage object.
 *
 * @exception   If the class Storage is not found the function throws the ClassNotFoundException. If the constructor of
 *              this class is not found the function throws the exception NotSuchMethodException.
 */
jobject jstorage_new_instance(JNIEnv * env);

/**
 * This function associated a native storage to a java storage instance.
 *
 * @param jstorage The java storage instance.
 * @param storage  The native storage to bind.
 * @param env      The environment of the current thread
 *
 * @exception      If the class Storage is not found the function throws the ClassNotFoundException. If the field bind
 *                 of this class is not found the function throws the exception NotSuchFieldException.
 */
void jstorage_bind(jobject jstorage, msg_storage_t storage, JNIEnv * env);

/**
 * This function returns a native storage from a java storage instance.
 *
 * @param jstorage  The java storage object from which get the native storage.
 * @param env       The environment of the current thread
 *
 * @return          The function returns the native storage associated to the java storage object.
 *
 * @exception       If the class Storage is not found the function throws the ClassNotFoundException. If the field bind
 *                  of this class is not found the function throws the exception NotSuchFieldException.
 */
msg_storage_t jstorage_get_native(JNIEnv * env, jobject jstorage);

/**
 * Class      org_simgrid_msg_Storage
 * Method      nativeInit
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Storage_nativeInit(JNIEnv*, jclass);

/**
 * This function returns a global reference to the  java storage instance specified by the parameter jstorage.
 *
 * @param jstorage   The original java storage instance.
 * @param env        The environment of the current thread
 *
 * @return           The global reference to the original java storage instance.
 */
jobject jstorage_ref(JNIEnv * env, jobject jstorage);

/**
 * This function delete a global reference to a java storage instance.
 *
 * @param        The global reference to delete.
 * @param env      The environment of the current thread
 */
void jstorage_unref(JNIEnv * env, jobject jstorage);

/**
 * This function returns the name of a MSG storage.
 *
 * @param jstorage      A java storage object.
 * @param env      The environment of the current thread
 *
 * @return        The name of the storage.
 */
const char *jstorage_get_name(jobject jstorage, JNIEnv * env);

/*
 * Class    org_simgrid_msg_Storage
 * Method    getByName
 * Signature  (Ljava/lang/String;)Lsimgrid/msg/Storage;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getByName(JNIEnv *, jclass, jstring);

/*
 * Class    org_simgrid_msg_Storage
 * Method    getSize
 * Signature  ()D
 */
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getSize(JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Storage
 * Method    getFreeSize
 * Signature  ()D
 */
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getFreeSize(JNIEnv *, jobject);

/*
 * Class    org_simgrid_msg_Storage
 * Method    getUsedSize
 * Signature  ()D
 */
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getUsedSize(JNIEnv *, jobject);

/*
 * Class        org_simgrid_msg_Storage
 * Method       getProperty
 * Signature    (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getProperty(JNIEnv *env, jobject jstorage, jobject jname);

/*
 * Class        org_simgrid_msg_Storage
 * Method       setProperty
 * Signature    (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Storage_setProperty(JNIEnv *env, jobject jstorage, jobject jname, jobject jvalue);

/*
 * Class        org_simgrid_msg_Storage
 * Method       getHost
 * Signature    (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getHost(JNIEnv * env,jobject jstorage);

/**
 * Class org_simgrid_msg_Storage
 * Method all
 */
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Storage_all(JNIEnv *, jclass);

SG_END_DECL()
#endif                          /*!MSG_JSTORAGE_H */
