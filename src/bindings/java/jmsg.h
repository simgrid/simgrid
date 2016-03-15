/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG4JAVA_H
#define MSG4JAVA_H
#include <simgrid/msg.h>
#include <jni.h>

SG_BEGIN_DECL()

extern int JAVA_HOST_LEVEL;
extern int JAVA_STORAGE_LEVEL;

JavaVM *get_java_VM(void);
JNIEnv *get_current_thread_env(void);
/**
 * This function throws the correct exception according to the status provided.
 */
void jmsg_throw_status(JNIEnv *env, msg_error_t status);

/*
 * Class    org_simgrid_msg_Msg
 * Method    getClock
 * Signature  ()D
 */
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Msg_getClock(JNIEnv *, jclass);

/**
 * Class    org_simgrid_msg_Msg
 * Method    run
 */
JNIEXPORT void JNICALL JNICALL Java_org_simgrid_msg_Msg_run(JNIEnv * env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_init(JNIEnv * env, jclass cls, jobjectArray jargs);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_energyInit(void);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_debug(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_verb(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_info(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_warn(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_error(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_critical(JNIEnv *, jclass, jstring);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_createEnvironment(JNIEnv * env, jclass cls, jstring jplatformFile);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Msg_environmentGetRoutingRoot(JNIEnv * env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_deployApplication(JNIEnv * env, jclass cls, jstring jdeploymentFile);

SG_END_DECL()
#endif                          /* !MSG4JAVA_H */
