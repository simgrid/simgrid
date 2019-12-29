/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/load.h"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"

#include "JavaContext.hpp"
#include "jmsg.hpp"
#include "jmsg_host.h"
#include "jmsg_storage.h"
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
  const char *name = env->GetStringUTFChars(jname, 0);
  /* get the host by name       (the hosts are created during the grid resolution) */
  msg_host_t host = MSG_host_by_name(name);

  if (not host) { /* invalid name */
    jxbt_throw_host_not_found(env, name);
    env->ReleaseStringUTFChars(jname, name);
    return nullptr;
  }
  env->ReleaseStringUTFChars(jname, name);

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
    host->extension_set(JAVA_HOST_LEVEL, (void *)jhost);
  }

  /* return the global reference to the java host instance */
  return (jobject) host->extension(JAVA_HOST_LEVEL);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_currentHost(JNIEnv * env, jclass cls) {
  jobject jhost;

  msg_host_t host = MSG_host_self();

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
    host->extension_set(JAVA_HOST_LEVEL, (void *) jhost);
  } else {
    jhost = (jobject) host->extension(JAVA_HOST_LEVEL);
  }

  return jhost;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_on(JNIEnv *env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);
  MSG_host_on(host);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_off(JNIEnv *env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);
  if (not simgrid::ForcefulKillException::try_n_catch([host]() { MSG_host_off(host); }))
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", "Host turned off");
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getCount(JNIEnv * env, jclass cls) {
  xbt_dynar_t hosts =  MSG_hosts_as_dynar();
  int nb_host = xbt_dynar_length(hosts);
  xbt_dynar_free(&hosts);
  return (jint) nb_host;
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getSpeed(JNIEnv * env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_host_get_speed(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCoreNumber(JNIEnv * env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_host_get_core_number(host);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getProperty(JNIEnv *env, jobject jhost, jobject jname) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return nullptr;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);

  const char *property = MSG_host_get_property_value(host, name);
  if (not property) {
    return nullptr;
  }

  jobject jproperty = env->NewStringUTF(property);

  env->ReleaseStringUTFChars((jstring) jname, name);

  return jproperty;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Host_setProperty(JNIEnv *env, jobject jhost, jobject jname, jobject jvalue) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);
  const char *value_java = env->GetStringUTFChars((jstring) jvalue, 0);
  const char* value      = xbt_strdup(value_java);

  MSG_host_set_property_value(host, name, value);

  env->ReleaseStringUTFChars((jstring) jvalue, value_java);
  env->ReleaseStringUTFChars((jstring) jname, name);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Host_isOn(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return (jboolean) MSG_host_is_on(host);
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getMountedStorage(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);
  jobject jstorage;
  jstring jname;

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  int index = 0;
  jobjectArray jtable;
  std::unordered_map<std::string, msg_storage_t> mounted_storages = host->get_mounted_storages();
  int count  = mounted_storages.size();
  jclass cls = env->FindClass("org/simgrid/msg/Storage");

  jtable = env->NewObjectArray((jsize) count, cls, nullptr);

  if (not jtable) {
    jxbt_throw_jni(env, "Storages table allocation failed");
    return nullptr;
  }

  for (auto const& elm : mounted_storages) {
    jname    = env->NewStringUTF(elm.second->get_cname());
    jstorage = Java_org_simgrid_msg_Storage_getByName(env,cls,jname);
    env->SetObjectArrayElement(jtable, index, jstorage);
    index++;
  }
  return jtable;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getAttachedStorage(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }
  jobjectArray jtable;

  xbt_dynar_t dyn = sg_host_get_attached_storage_list(host);
  jclass cls      = jxbt_get_class(env, "java/lang/String");
  jtable          = env->NewObjectArray(static_cast<jsize>(xbt_dynar_length(dyn)), cls, nullptr);
  unsigned int index;
  const char* storage_name;
  jstring jstorage_name;
  xbt_dynar_foreach (dyn, index, storage_name) {
    jstorage_name = env->NewStringUTF(storage_name);
    env->SetObjectArrayElement(jtable, index, jstorage_name);
  }
  xbt_dynar_free_container(&dyn);
  return jtable;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getStorageContent(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }
  return (jobjectArray)MSG_host_get_storage_content(host);
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_all(JNIEnv * env, jclass cls_arg)
{
  xbt_dynar_t table =  MSG_hosts_as_dynar();
  int count = xbt_dynar_length(table);

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");
  if (not cls)
    return nullptr;

  jobjectArray jtable = env->NewObjectArray((jsize)count, cls, nullptr);

  if (not jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  for (int index = 0; index < count; index++) {
    auto const* host = xbt_dynar_get_as(table, index, msg_host_t);
    jobject jhost   = static_cast<jobject>(host->extension(JAVA_HOST_LEVEL));

    if (not jhost) {
      jstring jname = env->NewStringUTF(host->get_cname());
      jhost         = Java_org_simgrid_msg_Host_getByName(env, cls_arg, jname);
    }

    env->SetObjectArrayElement(jtable, index, jhost);
  }
  xbt_dynar_free(&table);
  return jtable;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setAsyncMailbox(JNIEnv * env, jclass cls_arg, jobject jname)
{
  const char *name = env->GetStringUTFChars((jstring) jname, 0);
  sg_mailbox_set_receiver(name);
  env->ReleaseStringUTFChars((jstring) jname, name);
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

  return MSG_host_get_consumed_energy(host);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setPstate(JNIEnv* env, jobject jhost, jint pstate)
{
  msg_host_t host = jhost_get_native(env, jhost);
  MSG_host_set_pstate(host, pstate);
}
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getPstate(JNIEnv* env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_pstate(host);
}
JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getPstatesCount(JNIEnv* env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_nb_pstates(host);
}
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCurrentPowerPeak(JNIEnv* env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_speed(host);
}
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getPowerPeakAt(JNIEnv* env, jobject jhost, jint pstate)
{
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_power_peak_at(host, pstate);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getLoad(JNIEnv* env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_load(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCurrentLoad (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return MSG_host_get_current_load(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getComputedFlops (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return MSG_host_get_computed_flops(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getAvgLoad (JNIEnv *env, jobject jhost)
{
  const_sg_host_t host = jhost_get_native(env, jhost);

  if (not host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return MSG_host_get_avg_load(host);
}
