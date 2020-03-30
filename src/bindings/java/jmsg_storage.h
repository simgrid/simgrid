/* Java bindings of the Storage API.                                        */

/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JSTORAGE_H
#define MSG_JSTORAGE_H

#include "simgrid/storage.h"
#include <jni.h>

SG_BEGIN_DECL

/** Returns a new java instance of a storage. */
jobject jstorage_new_instance(JNIEnv * env);

/** Binds a native instance to a java instance. */
void jstorage_bind(jobject jstorage, sg_storage_t storage, JNIEnv* env);

/** Extracts the native instance associated to a java instance. */
sg_storage_t jstorage_get_native(JNIEnv* env, jobject jstorage);

/** Initialize the native world, called from the Java world at startup */
JNIEXPORT void JNICALL Java_org_simgrid_msg_Storage_nativeInit(JNIEnv *env, jclass cls);

/** Take a ref onto the java instance (to prevent its collection) */
jobject jstorage_ref(JNIEnv * env, jobject jstorage);

/** Release a ref onto the java instance */
void jstorage_unref(JNIEnv * env, jobject jstorage);

/* Implement the Java API */

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getByName(JNIEnv* env, jclass cls, jstring jname);
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getSize(JNIEnv *env, jobject jstorage);
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getFreeSize(JNIEnv *env, jobject jstorage);
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getUsedSize(JNIEnv *env, jobject jstorage);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getProperty(JNIEnv *env, jobject jstorage, jobject jname);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Storage_setProperty(JNIEnv* env, jobject jstorage, jobject jname,
                                                                jobject jvalue);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getHost(JNIEnv * env,jobject jstorage);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Storage_all(JNIEnv *env, jclass cls);

SG_END_DECL
#endif
