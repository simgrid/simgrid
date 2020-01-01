/* Java bindings of the Trace API.                                          */

/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <jni.h>
/* Header for class org_simgrid_trace_Trace */

#ifndef Included_org_simgrid_trace_Trace
#define Included_org_simgrid_trace_Trace

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableDeclare (JNIEnv *env, jclass cls, jstring jvar);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableDeclare (JNIEnv *env, jclass cls, jstring jvar);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableDeclareWithColor (JNIEnv *env, jclass cls, jstring jvar,
                                                                                  jstring jcolor);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSet (JNIEnv *env, jclass cls, jstring js_host,
                                                                     jstring jvar, jdouble value);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableSet (JNIEnv *env, jclass cls, jstring js_wn,
                                                                   jstring jvar, jdouble value);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableAdd (JNIEnv *env, jclass cls, jstring js_host,
                                                                     jstring jvar, jdouble value);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSub (JNIEnv *env, jclass cls, jstring js_host,
                                                                     jstring jvar, jdouble value);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_trace_Trace_getHostVariablesName (JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclare (JNIEnv *env, jclass cls, jstring jvar);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclareWithColor (JNIEnv *env, jclass cls, jstring jvar,
                                                                                  jstring jcolor);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSet (JNIEnv *env, jclass cls, jstring jlink,
                                                                     jstring jvar, jdouble jvalue);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableAdd (JNIEnv *env, jclass cls, jstring jlink,
                                                                     jstring jvar, jdouble jvalue);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSub (JNIEnv *env, jclass cls, jstring jlink,
                                                                     jstring jvar, jdouble jvalue);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSet (JNIEnv *env, jclass cls, jstring jsrc,
                                                                           jstring jdst, jstring jvar, jdouble jvalue);
JNIEXPORT jobjectArray JNICALL Java_org_simgrid_trace_Trace_getLinkVariablesName (JNIEnv *env, jclass cls);

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclare(JNIEnv * env, jclass cls, jstring js);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclareValue (JNIEnv *env, jclass cls, jstring js_state,
                                                                           jstring js_value, jstring js_color);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostSetState (JNIEnv *env, jclass cls, jstring js_host,
                                                                  jstring js_state, jstring js_value);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPushState (JNIEnv *env, jclass cls, jstring js_host,
                                                                   jstring js_state, jstring js_value);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPopState (JNIEnv *env, jclass cls, jstring js_host,
                                                                  jstring js_state);

/* Missing calls:
Java_org_simgrid_trace_Trace_linkVariableSetWithTime
Java_org_simgrid_trace_Trace_linkVariableAddWithTime
Java_org_simgrid_trace_Trace_linkVariableSubWithTime
Java_org_simgrid_trace_Trace_linkSrcDstVariableAdd
Java_org_simgrid_trace_Trace_linkSrcDstVariableSub
Java_org_simgrid_trace_Trace_linkSrcDstVariableSetWithTime
Java_org_simgrid_trace_Trace_linkSrcdstVariableAddWithTime
Java_org_simgrid_trace_Trace_linkSrcDstVariableSubWithTime
*/
#ifdef __cplusplus
}
#endif
#endif
