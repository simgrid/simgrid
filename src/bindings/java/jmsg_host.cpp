/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/load.h"
#include "simgrid/s4u/Host.hpp"

#include "JavaContext.hpp"
#include "jmsg.hpp"
#include "jmsg_host.h"
#include "jxbt_utilities.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

static jmethodID jhost_method_Host_constructor;
static jfieldID jhost_field_Host_bind;
static jfieldID jhost_field_Host_name;

jobject jhost_new_instance(JNIEnv * env) {
  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");
  return env->NewObject(cls, jhost_method_Host_constructor);
}

jobject jhost_ref(JNIEnv * env, jobject jhost) {
  return env->NewGlobalRef(jhost);
}

void jhost_unref(JNIEnv * env, jobject jhost) {
  env->DeleteGlobalRef(jhost);
}

void jhost_bind(jobject jhost, msg_host_t host, JNIEnv * env) {
  env->SetLongField(jhost, jhost_field_Host_bind, (jlong) (uintptr_t) (host));
}

msg_host_t jhost_get_native(JNIEnv * env, jobject jhost) {
  return (msg_host_t) (uintptr_t) env->GetLongField(jhost, jhost_field_Host_bind);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_Host = env->FindClass("org/simgrid/msg/Host");
  jhost_method_Host_constructor = env->GetMethodID(class_Host, "<init>", "()V");
  jhost_field_Host_bind = jxbt_get_jfield(env,class_Host, "bind", "J");
  jhost_field_Host_name = jxbt_get_jfield(env, class_Host, "name", "Ljava/lang/String;");
  xbt_assert(class_Host && jhost_field_Host_name && jhost_method_Host_constructor && jhost_field_Host_bind,
             "Native initialization of msg/Host failed. Please report that bug");
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getByName(JNIEnv* env, jclass cls, jstring jname)
{
  /* get the C string from the java string */
  if (jname == nullptr) {
    jxbt_throw_null(env, "No host can have a null name");
    return nullptr;
  }
  jstring_wrapper name(env, jname);
  /* get the host by name       (the hosts are created during the grid resolution) */
  sg_host_t host = sg_host_by_name(name);

  if (not host) { /* invalid name */
    jxbt_throw_host_not_found(env, name);
    return nullptr;
  }

  if (not host->extension(JAVA_HOST_LEVEL)) { /* native host not associated yet with java host */
    /* Instantiate a new java host */
    jobject jhost = jhost_new_instance(env);

    if (not jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return nullptr;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (not jhost) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return nullptr;
    }
    /* Sets the java host name */
    env->SetObjectField(jhost, jhost_field_Host_name, jname);
    /* bind the java host and the native host */
    jhost_bind(jhost, host, env);

    /* the native host data field is set with the global reference to the java host returned by this function */
    host->extension_set(JAVA_HOST_LEVEL, jhost);
  }

  /* return the global reference to the java host instance */
  return (jobject) host->extension(JAVA_HOST_LEVEL);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_currentHost(JNIEnv * env, jclass cls) {
  jobject jhost;

  sg_host_t host = sg_host_self();

  if (not host->extension(JAVA_HOST_LEVEL)) {
    /* the native host not yet associated with the java host instance */

    /* instantiate a new java host instance */
    jhost = jhost_new_instance(env);

    if (not jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return nullptr;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (not jhost) {
      jxbt_throw_jni(env, "global ref allocation failed");
      return nullptr;
    }
    /* Sets the host name */
    jobject jname = env->NewStringUTF(host->get_cname());
    env->SetObjectField(jhost, jhost_field_Host_name, jname);
    /* Bind & store it */
    jhost_bind(jhost, host, env);
    host->extension_set(JAVA_HOST_LEVEL, jhost);
  } else {
    jhost = (jobject) host->extension(JAVA_HOST_LEVEL);
  }

  return jhost;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_on(JNIEnv *env, jobject jhost) {
  sg_host_t host = jhost_get_native(env, jhost);
  sg_host_turn_on(host);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_off(JNIEnv *env, jobject jhost) {
  sg_host_t host = jhost_get_native(env, jhost);
  if (not simgrid::ForcefulKillException::try_n_catch([host]() { sg_host_turn_off(host); }))
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", "Host turned off");
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getCount(JNIEnv * env, jclass cls) {
  return (jint)sg_host_count();
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getSpeed(JNIEnv * env, jobject jhost) {
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble)sg_host_get_speed(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCoreNumber(JNIEnv * env, jobject jhost) {
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble)sg_host_core_count(host);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getProperty(JNIEnv *env, jobject jhost, jobject jname) {
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return nullptr;
  }
  jstring_wrapper name(env, (jstring)jname);

  const char* property = sg_host_get_property_value(host, name);
  if (not property) {
    return nullptr;
  }

  jobject jproperty = env->NewStringUTF(property);

  return jproperty;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Host_setProperty(JNIEnv *env, jobject jhost, jobject jname, jobject jvalue) {
  sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }
  jstring_wrapper name(env, (jstring)jname);
  jstring_wrapper value_java(env, (jstring)jvalue);
  const char* value      = xbt_strdup(value_java);

  sg_host_set_property_value(host, name, value);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Host_isOn(JNIEnv * env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return (jboolean)sg_host_is_on(host);
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_all(JNIEnv * env, jclass cls_arg)
{
  sg_host_t* table = sg_host_list();
  int count        = sg_host_count();

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");
  if (not cls)
    return nullptr;

  jobjectArray jtable = env->NewObjectArray((jsize)count, cls, nullptr);

  if (not jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  for (int index = 0; index < count; index++) {
    auto jhost = static_cast<jobject>(table[index]->extension(JAVA_HOST_LEVEL));

    if (not jhost) {
      jstring jname = env->NewStringUTF(table[index]->get_cname());
      jhost         = Java_org_simgrid_msg_Host_getByName(env, cls_arg, jname);
    }

    env->SetObjectArrayElement(jtable, index, jhost);
  }
  xbt_free(table);
  return jtable;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setAsyncMailbox(JNIEnv * env, jclass cls_arg, jobject jname)
{
  jstring_wrapper name(env, (jstring)jname);
  sg_mailbox_set_receiver(name);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_updateAllEnergyConsumptions(JNIEnv* env, jclass cls)
{
  sg_host_energy_update_all();
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getConsumedEnergy (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return sg_host_get_consumed_energy(host);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setPstate(JNIEnv* env, jobject jhost, jint pstate)
{
  sg_host_t host = jhost_get_native(env, jhost);
  sg_host_set_pstate(host, pstate);
}
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getPstate(JNIEnv* env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);
  return sg_host_get_pstate(host);
}
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getPstatesCount(JNIEnv* env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);
  return sg_host_get_nb_pstates(host);
}
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCurrentPowerPeak(JNIEnv* env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);
  return sg_host_get_speed(host);
}
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getPowerPeakAt(JNIEnv* env, jobject jhost, jint pstate)
{
  const_sg_host_t host = jhost_get_native(env, jhost);
  return sg_host_get_pstate_speed(host, pstate);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getLoad(JNIEnv* env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);
  return sg_host_get_load(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCurrentLoad (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return sg_host_get_current_load(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getComputedFlops (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return sg_host_get_computed_flops(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getAvgLoad (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return sg_host_get_avg_load(host);
}
