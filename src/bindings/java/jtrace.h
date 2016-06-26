/* Copyright (c) 2013-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <jni.h>
/* Header for class org_simgrid_trace_Trace */

#ifndef _Included_org_simgrid_trace_Trace
#define _Included_org_simgrid_trace_Trace
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableDeclare (JNIEnv *env, jclass klass, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableDeclare (JNIEnv *env, jclass klass, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableDeclareWithColor (JNIEnv *env, jclass klass, jstring, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSet (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableSet (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableAdd (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSub (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSetWithTime (JNIEnv *env, jclass klass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableAddWithTime (JNIEnv *env, jclass klass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSubWithTime (JNIEnv *env, jclass klass, jdouble, jstring, jstring, jdouble);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_trace_Trace_getHostVariablesName (JNIEnv *env, jclass klass);

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclare (JNIEnv *env, jclass klass, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclareWithColor (JNIEnv *env, jclass klass, jstring, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSet (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableAdd (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSub (JNIEnv *env, jclass klass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSetWithTime (JNIEnv *env, jclass klass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableAddWithTime (JNIEnv *env, jclass klass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSubWithTime (JNIEnv *env, jclass klass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSet (JNIEnv *env, jclass klass, jstring, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableAdd (JNIEnv *env, jclass klass, jstring, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSub (JNIEnv *env, jclass klass, jstring, jstring, jstring, jdouble);

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSetWithTime (JNIEnv *env, jclass klass, jdouble, jstring,
                                                                                   jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcdstVariableAddWithTime (JNIEnv *env, jclass klass, jdouble, jstring,
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSubWithTime (JNIEnv *env, jclass klass, jdouble, jstring,
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_trace_Trace_getLinkVariablesName (JNIEnv *env, jclass klass);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclare (JNIEnv *env, jclass klass, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclareValue (JNIEnv *env, jclass klass, jstring, jstring, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostSetState (JNIEnv *env, jclass klass, jstring, jstring, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPushState (JNIEnv *env, jclass klass, jstring, jstring, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPopState (JNIEnv *env, jclass klass, jstring, jstring);

#ifdef __cplusplus
}
#endif
#endif
