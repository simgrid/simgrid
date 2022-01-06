/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JMSG_HPP
#define JMSG_HPP
#include <jni.h>
#include <simgrid/msg.h>
#include <unordered_map>

extern "C" {

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

extern int JAVA_HOST_LEVEL;

JNIEnv* get_current_thread_env();
/**
 * This function throws the correct exception according to the status provided.
 */
void jmsg_throw_status(JNIEnv* env, msg_error_t status);

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Msg_getClock(JNIEnv* env, jclass cls);
JNIEXPORT void JNICALL JNICALL Java_org_simgrid_msg_Msg_run(JNIEnv* env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_init(JNIEnv* env, jclass cls, jobjectArray jargs);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_energyInit();
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_liveMigrationInit();
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_fileSystemInit();
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_loadInit();

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_debug(JNIEnv* env, jclass cls, jstring jargs);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_verb(JNIEnv* env, jclass cls, jstring jargs);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_info(JNIEnv* env, jclass cls, jstring jargs);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_warn(JNIEnv* env, jclass cls, jstring jargs);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_error(JNIEnv* env, jclass cls, jstring jargs);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_critical(JNIEnv* env, jclass cls, jstring jargs);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_createEnvironment(JNIEnv* env, jclass cls, jstring jplatformFile);
JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Msg_environmentGetRoutingRoot(JNIEnv* env, jclass cls);
JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_deployApplication(JNIEnv* env, jclass cls, jstring jdeploymentFile);
}
#endif
