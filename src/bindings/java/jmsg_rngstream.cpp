/* Java binding of the RngStream library                                    */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"

#include "jmsg_rngstream.h"
#include "jxbt_utilities.hpp"

jfieldID jrngstream_bind;

RngStream jrngstream_to_native(JNIEnv *env, jobject jrngstream) {
  RngStream rngstream = (RngStream)(intptr_t)env->GetLongField(jrngstream, jrngstream_bind);
  if (not rngstream) {
    jxbt_throw_notbound(env, "rngstream", jrngstream);
    return nullptr;
  }
  return rngstream;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_RngStream = env->FindClass("org/simgrid/msg/RngStream");

  jrngstream_bind = jxbt_get_jfield(env, class_RngStream, "bind", "J");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_create(JNIEnv *env, jobject jrngstream, jstring jname) {
  const char *name = env->GetStringUTFChars(jname, 0);
  RngStream rngstream = RngStream_CreateStream(name);
  //Bind the RngStream object
  env->SetLongField(jrngstream, jrngstream_bind, (intptr_t)rngstream);

  env->ReleaseStringUTFChars(jname, name);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_nativeFinalize(JNIEnv *env, jobject jrngstream) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  RngStream_DeleteStream(&rngstream);
  env->SetLongField(jrngstream, jrngstream_bind, (intptr_t)nullptr);
}

JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_RngStream_setPackageSeed(JNIEnv *env, jobject jrngstream, jintArray jseed) {

  if (jseed == nullptr) {
    jxbt_throw_null(env, xbt_strdup("seed argument is null"));
    return JNI_FALSE;
  }

  jint buffer[6];
  env->GetIntArrayRegion(jseed, 0, 6, buffer);

  // The C API expects unsigned long which are wider than int on LP64.
  // We need to convert:
  unsigned long seed[6];
  for (int i = 0; i != 6; ++i)
    seed[i] = buffer[i];

  int result = RngStream_SetPackageSeed(seed);
  return result == -1 ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_resetStart(JNIEnv *env, jobject jrngstream) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return;

  RngStream_ResetStartStream(rngstream);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_resetStartSubstream(JNIEnv *env, jobject jrngstream) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return;

  RngStream_ResetStartSubstream(rngstream);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_resetNextSubstream(JNIEnv *env, jobject jrngstream) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return;

  RngStream_ResetNextSubstream(rngstream);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_setAntithetic(JNIEnv *env, jobject jrngstream, jboolean ja) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return;

  if (ja == JNI_TRUE) {
    RngStream_SetAntithetic(rngstream,-1);
  }
  else {
    RngStream_SetAntithetic(rngstream,0);
  }
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_RngStream_setSeed(JNIEnv *env, jobject jrngstream, jintArray jseed) {
  jint buffer[6];

  env->GetIntArrayRegion(jseed, 0, 6, buffer);

  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return JNI_FALSE;

  // The C API expects unsigned long which are wider than int on LP64.
  // We need to convert:
  unsigned long seed[6];
  for (int i = 0; i != 6; ++i)
    seed[i] = buffer[i];

  int result = RngStream_SetSeed(rngstream, seed);

  return result == -1 ? JNI_FALSE : JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_RngStream_advanceState(JNIEnv *env, jobject jrngstream, jint e, jint g) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return;

  RngStream_AdvanceState(rngstream, (long)e, (long)g);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_RngStream_randU01(JNIEnv *env, jobject jrngstream) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return 0;

  return (jdouble)RngStream_RandU01(rngstream);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_RngStream_randInt(JNIEnv *env, jobject jrngstream, jint i, jint j) {
  RngStream rngstream = jrngstream_to_native(env, jrngstream);
  if (not rngstream)
    return 0;

  return (jint)RngStream_RandInt(rngstream, (int)i, (int)j);
}
