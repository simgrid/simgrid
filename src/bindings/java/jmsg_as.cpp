/* Java bindings of the NetZones.                                           */

/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/NetZone.hpp"

#include "jmsg.hpp"
#include "jmsg_as.hpp"
#include "jmsg_host.h"
#include "jxbt_utilities.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

static jmethodID jas_method_As_constructor;
static jfieldID jas_field_As_bind;

jobject jnetzone_new_instance(JNIEnv* env)
{
  jclass cls = jxbt_get_class(env, "org/simgrid/msg/As");
  return env->NewObject(cls, jas_method_As_constructor);
}

jobject jnetzone_ref(JNIEnv* env, jobject jas)
{
  return env->NewGlobalRef(jas);
}

void jnetzone_unref(JNIEnv* env, jobject jas)
{
  env->DeleteGlobalRef(jas);
}

void jnetzone_bind(jobject jas, simgrid::s4u::NetZone* netzone, JNIEnv* env)
{
  env->SetLongField(jas, jas_field_As_bind, (jlong)(uintptr_t)(netzone));
}

simgrid::s4u::NetZone* jnetzone_get_native(JNIEnv* env, jobject jas)
{
  return (simgrid::s4u::NetZone*)(uintptr_t)env->GetLongField(jas, jas_field_As_bind);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_As_nativeInit(JNIEnv* env, jclass cls)
{
  jclass class_As = env->FindClass("org/simgrid/msg/As");
  jas_method_As_constructor = env->GetMethodID(class_As, "<init>", "()V");
  jas_field_As_bind = jxbt_get_jfield(env,class_As, "bind", "J");
  xbt_assert(class_As && jas_method_As_constructor && jas_field_As_bind,
             "Native initialization of msg/AS failed. Please report that bug");
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getName(JNIEnv * env, jobject jas) {
  const simgrid::s4u::NetZone* as = jnetzone_get_native(env, jas);
  return env->NewStringUTF(as->get_cname());
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getSons(JNIEnv * env, jobject jas) {
  int index = 0;
  jobjectArray jtable;
  const simgrid::s4u::NetZone* self_as = jnetzone_get_native(env, jas);

  jclass cls = env->FindClass("org/simgrid/msg/As");

  if (not cls)
    return nullptr;

  jtable = env->NewObjectArray(static_cast<jsize>(self_as->get_children().size()), cls, nullptr);

  if (not jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  for (auto const& tmp_as : self_as->get_children()) {
    jobject tmp_jas = jnetzone_new_instance(env);
    if (not tmp_jas) {
      jxbt_throw_jni(env, "java As instantiation failed");
      return nullptr;
    }
    tmp_jas = jnetzone_ref(env, tmp_jas);
    if (not tmp_jas) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return nullptr;
    }
    jnetzone_bind(tmp_jas, tmp_as, env);

    env->SetObjectArrayElement(jtable, index, tmp_jas);
    index++;
  }
  return jtable;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getProperty(JNIEnv *env, jobject jas, jobject jname) {
  const simgrid::s4u::NetZone* as = jnetzone_get_native(env, jas);

  if (not as) {
    jxbt_throw_notbound(env, "as", jas);
    return nullptr;
  }
  const char* name = env->GetStringUTFChars(static_cast<jstring>(jname), nullptr);

  const char* property = sg_zone_get_property_value(as, name);
  if (not property) {
    return nullptr;
  }

  jobject jproperty = env->NewStringUTF(property);

  env->ReleaseStringUTFChars(static_cast<jstring>(jname), name);

  return jproperty;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getHosts(JNIEnv * env, jobject jas)
{
  jobjectArray jtable;
  jobject jhost;
  jstring jname;
  const simgrid::s4u::NetZone* as = jnetzone_get_native(env, jas);

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");
  if (not cls)
    return nullptr;

  std::vector<simgrid::s4u::Host*> table = as->get_all_hosts();

  jtable = env->NewObjectArray(static_cast<jsize>(table.size()), cls, nullptr);

  if (not jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  int index = 0;
  for (auto const& host : table) {
    jhost = static_cast<jobject>(host->extension(JAVA_HOST_LEVEL));
    if (not jhost) {
      jname = env->NewStringUTF(host->get_cname());

      jhost = Java_org_simgrid_msg_Host_getByName(env, cls, jname);

      env->ReleaseStringUTFChars(static_cast<jstring>(jname), host->get_cname());
    }

    env->SetObjectArrayElement(jtable, index, jhost);
    index++;
  }
  return jtable;
}
