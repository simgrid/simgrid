/* Java bindings of the Trace API.                                          */

/* Copyright (c) 2012-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jtrace.h"
#include "jxbt_utilities.hpp"
#include "simgrid/instr.h"

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclare(JNIEnv * env, jclass cls, jstring js)
{
  jstring_wrapper s(env, js);
  TRACE_host_state_declare(s);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostStateDeclareValue (JNIEnv *env, jclass cls, jstring js_state,
                                                                           jstring js_value, jstring js_color)
{
  jstring_wrapper state(env, js_state);
  jstring_wrapper value(env, js_value);
  jstring_wrapper color(env, js_color);

  TRACE_host_state_declare_value(state, value, color);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostSetState (JNIEnv *env, jclass cls, jstring js_host,
                                                                  jstring js_state, jstring js_value)
{
  jstring_wrapper host(env, js_host);
  jstring_wrapper state(env, js_state);
  jstring_wrapper value(env, js_value);

  TRACE_host_set_state(host, state, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPushState (JNIEnv *env, jclass cls, jstring js_host,
                                                                   jstring js_state, jstring js_value)
{
  jstring_wrapper host(env, js_host);
  jstring_wrapper state(env, js_state);
  jstring_wrapper value(env, js_value);

  TRACE_host_push_state(host, state, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostPopState (JNIEnv *env, jclass cls, jstring js_host,
                                                                  jstring js_state)
{
  jstring_wrapper host(env, js_host);
  jstring_wrapper state(env, js_state);

  TRACE_host_pop_state(host, state);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableDeclare(JNIEnv* env, jclass cls, jstring js_variable)
{
  jstring_wrapper variable(env, js_variable);
  simgrid::instr::declare_host_variable(variable);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSet(JNIEnv* env, jclass cls, jstring js_host,
                                                                    jstring js_variable, jdouble value)
{
  jstring_wrapper host(env, js_host);
  jstring_wrapper variable(env, js_variable);

  simgrid::instr::set_host_variable(host, variable, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableAdd(JNIEnv* env, jclass cls, jstring js_host,
                                                                    jstring js_variable, jdouble value)
{
  jstring_wrapper host(env, js_host);
  jstring_wrapper variable(env, js_variable);

  simgrid::instr::add_host_variable(host, variable, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_hostVariableSub(JNIEnv* env, jclass cls, jstring js_host,
                                                                    jstring js_variable, jdouble value)
{
  jstring_wrapper host(env, js_host);
  jstring_wrapper variable(env, js_variable);

  simgrid::instr::sub_host_variable(host, variable, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableDeclare(JNIEnv* env, jclass cls, jstring js_variable)
{
  jstring_wrapper variable(env, js_variable);
  simgrid::instr::declare_vm_variable(variable);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_vmVariableSet(JNIEnv* env, jclass cls, jstring js_vm,
                                                                  jstring js_variable, jdouble value)
{
  jstring_wrapper vm(env, js_vm);
  jstring_wrapper variable(env, js_variable);

  simgrid::instr::set_vm_variable(vm, variable, value);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclare (JNIEnv *env, jclass cls, jstring jvar) {
  jstring_wrapper variable(env, jvar);
  simgrid::instr::declare_link_variable(variable);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableDeclareWithColor (JNIEnv *env, jclass cls, jstring jvar, jstring jcolor) {
  jstring_wrapper variable(env, jvar);
  jstring_wrapper color(env, jcolor);
  simgrid::instr::declare_link_variable(variable, color);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkVariableSet (JNIEnv *env, jclass cls, jstring jlink, jstring jvar, jdouble jvalue) {
  jstring_wrapper link(env, jlink);
  jstring_wrapper variable(env, jvar);
  simgrid::instr::set_link_variable(link, variable, jvalue);
}

JNIEXPORT void JNICALL Java_org_simgrid_trace_Trace_linkSrcDstVariableSet
  (JNIEnv *env, jclass cls, jstring jsrc, jstring jdst, jstring jvar, jdouble jval)
{
  jstring_wrapper src(env, jsrc);
  jstring_wrapper dst(env, jdst);

  jstring_wrapper variable(env, jvar);
  simgrid::instr::set_link_variable(src, dst, variable, jval);
}
