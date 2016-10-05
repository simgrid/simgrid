/* Functions related to the MSG VM API. */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_VM_H
#define MSG_VM_H

#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL()

void jvm_bind(JNIEnv *env, jobject jvm, msg_vm_t vm);
msg_vm_t jvm_get_native(JNIEnv *env, jobject jvm);

/*
 * Class      org_simgrid_msg_VM
 * Method      nativeInit
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_nativeInit(JNIEnv *env, jclass);

/**
 * Class      org_simgrid_msg_VM
 * Method      isCreated
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isCreated(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isRunning
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isRunning(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isMigrating
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isMigrating(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isSuspended
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isSuspended(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isResuming
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isResuming(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isSuspended
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isSaving(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isSave
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isSaved(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      isResuming
 * Signature  ()B
 */
JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isRestoring(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      setBound
 * Signature  (D)B
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_setBound(JNIEnv *env, jobject jvm, jdouble bound);

/**
 * Class            org_simgrid_msg_VM
 * Method           create
 * Signature    ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_create(JNIEnv *env, jobject jvm, jobject jhost, jstring jname,
                                                      jint jncore, jint jramsize, jint jnetcap, jstring jdiskpath,
                                                      jint jdisksize, jint dprate, jint mig_netspeed);

/**
 * Class            org_simgrid_msg_VM
 * Method           destroy
 * Signature    ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_nativeFinalize(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      start
 * Signature  (I)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_start(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      nativeMigrate
 * Signature  (Lorg/simgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_internalmig(JNIEnv *env, jobject jvm, jobject jhost);

/**
 * Class      org_simgrid_msg_VM
 * Method      suspend
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_suspend(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      resume
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_resume(JNIEnv *env, jobject jvm);

/**
 * Class      org_simgrid_msg_VM
 * Method      shutdown
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_shutdown(JNIEnv *env, jobject jvm);

/**
 * Class            org_simgrid_msg_VM
 * Method           save
 * Signature    ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_save(JNIEnv *env, jobject jvm);

/**
 * Class            org_simgrid_msg_VM
 * Method           restore
 * Signature    ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_restore(JNIEnv *env, jobject jvm);

SG_END_DECL()
#endif
