/* Functions related to the RngStream Java port                         */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_RNGSTREAM_H
#define MSG_RNGSTREAM_H
#include <jni.h>
#include <xbt/RngStream.h>

SG_BEGIN_DECL()

RngStream jrngstream_to_native(JNIEnv *env, jobject jrngstream);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_nativeInit(JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_create(JNIEnv *env, jobject jrngstream, jstring name);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_nativeFinalize(JNIEnv *env, jobject jrngstream);

JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_RngStream_setPackageSeed(JNIEnv *env, jobject jrngstream, jintArray seed);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_resetStart(JNIEnv *env, jobject jrngstream);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_resetStartSubstream(JNIEnv *env, jobject jrngstream);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_resetNextSubstream(JNIEnv *env, jobject jrngstream);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_setAntithetic(JNIEnv *env, jobject jrngstream, jboolean ja);

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_RngStream_setSeed(JNIEnv *env, jobject jrngstream, jintArray jseed);

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_advanceState(JNIEnv *env, jobject jrngstream, jint e, jint g);

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_RngStream_randU01(JNIEnv *env, jobject jrngstream);

JNIEXPORT jint JNICALL Java_org_simgrid_msg_RngStream_randInt(JNIEnv *env, jobject jrngstream, jint i, jint j);

SG_END_DECL()
#endif                          /* MSG_RNGSTREAM_H */
