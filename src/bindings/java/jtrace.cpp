/* Java Wrappers to the TRACE API.                                           */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// Please note, this file strongly relies on the jmsg.cpp,
// It will be great that a JNI expert gives a look to validate it - Adrien ;)

#include "jtrace.h"
#include <simgrid/instr.h>

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

// Define a new category
XBT_LOG_NEW_DEFAULT_CATEGORY (jtrace, "TRACE for Java(TM)");

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclare(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  TRACE_host_state_declare(s);  
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclareValue (JNIEnv *env, jclass cls, jstring js_state,
                                                                           jstring js_value, jstring js_color)
{
  const char *state = env->GetStringUTFChars(js_state, 0);
  const char *value = env->GetStringUTFChars(js_value, 0);
  const char *color = env->GetStringUTFChars(js_color, 0);

  TRACE_host_state_declare_value(state, value, color);  

  env->ReleaseStringUTFChars(js_state, state);
  env->ReleaseStringUTFChars(js_value, value);
  env->ReleaseStringUTFChars(js_color, color);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostSetState (JNIEnv *env, jclass cls, jstring js_host,
                                                                  jstring js_state, jstring js_value)
{
  const char *host = env->GetStringUTFChars(js_host, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);
  const char *value = env->GetStringUTFChars(js_value, 0);

  TRACE_host_set_state(host, state, value);  

  env->ReleaseStringUTFChars(js_host, host);
  env->ReleaseStringUTFChars(js_state, state);
  env->ReleaseStringUTFChars(js_value, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPushState (JNIEnv *env, jclass cls, jstring js_host,
                                                                   jstring js_state, jstring js_value)
{
  const char *host = env->GetStringUTFChars(js_host, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);
  const char *value = env->GetStringUTFChars(js_value, 0);

  TRACE_host_push_state(host, state, value);  

  env->ReleaseStringUTFChars(js_host, host);
  env->ReleaseStringUTFChars(js_state, state);
  env->ReleaseStringUTFChars(js_value, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPopState (JNIEnv *env, jclass cls, jstring js_host,
                                                                  jstring js_state)
{
  const char *host = env->GetStringUTFChars(js_host, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);

  TRACE_host_pop_state(host, state);  

  env->ReleaseStringUTFChars(js_host, host);
  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableDeclare (JNIEnv *env, jclass cls, jstring js_state)
{
  const char *state = env->GetStringUTFChars(js_state, 0);
  TRACE_host_variable_declare(state);
  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSet (JNIEnv *env, jclass cls, jstring js_host,
                                                                     jstring js_state, jdouble value)
{
  const char *host = env->GetStringUTFChars(js_host, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);

  TRACE_host_variable_set(host, state, value);

  env->ReleaseStringUTFChars(js_host, host);
  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSub (JNIEnv *env, jclass cls, jstring js_host,
                                                                     jstring js_state, jdouble value)
{
  const char *host = env->GetStringUTFChars(js_host, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);

  TRACE_host_variable_sub(host, state, value);

  env->ReleaseStringUTFChars(js_host, host);
  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableDeclare (JNIEnv *env, jclass cls, jstring js_state)
{
  const char *state = env->GetStringUTFChars(js_state, 0);

  TRACE_vm_variable_declare(state);

  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableSet (JNIEnv *env, jclass cls, jstring js_vm,
                                                                   jstring js_state, jdouble value)
{
  const char *vm = env->GetStringUTFChars(js_vm, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);

  TRACE_vm_variable_set(vm, state, value);

  env->ReleaseStringUTFChars(js_vm, vm);
  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableAdd (JNIEnv *env, jclass cls, jstring js_host,
                                                                     jstring js_state, jdouble value)
{
  const char *host = env->GetStringUTFChars(js_host, 0);
  const char *state = env->GetStringUTFChars(js_state, 0);

  TRACE_host_variable_set(host, state, value);

  env->ReleaseStringUTFChars(js_host, host);
  env->ReleaseStringUTFChars(js_state, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclare (JNIEnv *env, jclass cls, jstring js_var) {
  const char *variable = env->GetStringUTFChars(js_var, 0);
  TRACE_link_variable_declare (variable);
  env->ReleaseStringUTFChars(js_var, variable);
}
/* Missing calls
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclareWithColor (JNIEnv *, jclass, jstring, jstring);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSet (JNIEnv *, jclass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableAdd (JNIEnv *, jclass, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSub (JNIEnv *env, jclass cls, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSetWithTime (JNIEnv *, jclass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableAddWithTime (JNIEnv *, jclass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSubWithTime (JNIEnv *, jclass, jdouble, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSet (JNIEnv *, jclass, jstring, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableAdd (JNIEnv *, jclass, jstring, jstring, jstring, jdouble);
JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSub (JNIEnv *, jclass, jstring, jstring, jstring, jdouble);
*/
