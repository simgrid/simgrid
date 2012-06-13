/* Functions related to the MSG VM API. */

/* Copyright (c) 2012. The SimGrid Team. */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef MSG_VM_H
#define MSG_VM_H

#include <jni.h>
#include "msg/msg.h"

jfieldID jvm_field_bind;

void jvm_bind(JNIEnv *env, jobject jvm, msg_vm_t vm);
msg_vm_t jvm_get_native(JNIEnv *env, jobject jvm);

/*
 * Class			org_simgrid_msg_VM
 * Method			nativeInit
 * Signature	()V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_nativeInit(JNIEnv *env, jclass);
/**
 * Class			org_simgrid_msg_VM
 * Method			start
 * Signature	(I)V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_start(JNIEnv *env, jobject jvm, jobject jhost, jint jcoreamount);
/**
 * Class			org_simgrid_msg_VM
 * Method			all
 * Signature	()[Lorg/simgrid/msg/VM;
 */
JNIEXPORT jobjectArray JNICALL
Java_org_simgrid_msg_VM_all(JNIEnv *env, jclass cls);
/**
 * Class			org_simgrid_msg_VM
 * Method			isSuspended
 * Signature	()B
 */
JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_VM_isSuspended(JNIEnv *env, jobject jvm);
/**
 * Class			org_simgrid_msg_VM
 * Method			isRunning
 * Signature	()B
 */
JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_VM_isRunning(JNIEnv *env, jobject jvm);
/**
 * Class			org_simgrid_msg_VM
 * Method			bind
 * Signature	(Lorg/simgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_bind(JNIEnv *env, jobject jvm, jobject jprocess);
/**
 * Class			org_simgrid_msg_VM
 * Method			unbind
 * Signature	(Lorg/simgrid/msg/Process;)V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_unbind(JNIEnv *env, jobject jvm, jobject jprocess);
/**
 * Class			org_simgrid_msg_VM
 * Method			migrate
 * Signature	(Lorg/simgrid/msg/Host;)V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_migrate(JNIEnv *env, jobject jvm, jobject jhost);
/**
 * Class			org_simgrid_msg_VM
 * Method			suspend
 * Signature	()V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_suspend(JNIEnv *env, jobject jvm);
/**
 * Class			org_simgrid_msg_VM
 * Method			resume
 * Signature	()V
 */
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_resume(JNIEnv *env, jobject jvm);
#endif
