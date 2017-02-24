/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/str.h>
#include <xbt/dict.h>
#include <xbt/dynar.h>

#include "simgrid/s4u/NetZone.hpp"
#include <simgrid/s4u/host.hpp>

#include "simgrid/msg.h"
#include "jmsg_as.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"
#include "jmsg.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

SG_BEGIN_DECL()

static jmethodID jas_method_As_constructor;
static jfieldID jas_field_As_bind;

jobject jas_new_instance(JNIEnv * env) {
  jclass cls = jxbt_get_class(env, "org/simgrid/msg/As");
  return env->NewObject(cls, jas_method_As_constructor);
}

jobject jas_ref(JNIEnv * env, jobject jas) {
  return env->NewGlobalRef(jas);
}

void jas_unref(JNIEnv * env, jobject jas) {
  env->DeleteGlobalRef(jas);
}

void jas_bind(jobject jas, simgrid::s4u::NetZone* netzone, JNIEnv* env)
{
  env->SetLongField(jas, jas_field_As_bind, (jlong)(uintptr_t)(netzone));
}

simgrid::s4u::NetZone* jas_get_native(JNIEnv* env, jobject jas)
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
  simgrid::s4u::NetZone* as = jas_get_native(env, jas);
  return env->NewStringUTF(as->name());
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getSons(JNIEnv * env, jobject jas) {
  int index = 0;
  jobjectArray jtable;
  jobject tmp_jas;
  simgrid::s4u::NetZone* tmp_as;
  simgrid::s4u::NetZone* self_as = jas_get_native(env, jas);

  xbt_dict_t dict = self_as->children();
  int count = xbt_dict_length(dict);
  jclass cls = env->FindClass("org/simgrid/msg/As");

  if (!cls)
    return nullptr;

  jtable = env->NewObjectArray(static_cast<jsize>(count), cls, nullptr);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  xbt_dict_cursor_t cursor=nullptr;
  char *key;

  xbt_dict_foreach(dict,cursor,key,tmp_as) {
    tmp_jas = jas_new_instance(env);
    if (!tmp_jas) {
      jxbt_throw_jni(env, "java As instantiation failed");
      return nullptr;
    }
    tmp_jas = jas_ref(env, tmp_jas);
    if (!tmp_jas) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return nullptr;
    }
    jas_bind(tmp_jas, tmp_as, env);

    env->SetObjectArrayElement(jtable, index, tmp_jas);
    index++;
  }
  return jtable;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getProperty(JNIEnv *env, jobject jas, jobject jname) {
  simgrid::s4u::NetZone* as = jas_get_native(env, jas);

  if (!as) {
    jxbt_throw_notbound(env, "as", jas);
    return nullptr;
  }
  const char *name = env->GetStringUTFChars(static_cast<jstring>(jname), 0);

  const char *property = MSG_environment_as_get_property_value(as, name);
  if (!property) {
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
  msg_host_t host;
  simgrid::s4u::NetZone* as = jas_get_native(env, jas);

  xbt_dynar_t table = as->hosts();
  int count = xbt_dynar_length(table);

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");

  if (!cls)
    return nullptr;

  jtable = env->NewObjectArray(static_cast<jsize>(count), cls, nullptr);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  for (int index = 0; index < count; index++) {
    host = xbt_dynar_get_as(table,index,msg_host_t);

    jhost = static_cast<jobject>(host->extension(JAVA_HOST_LEVEL));
    if (!jhost) {
      jname = env->NewStringUTF(host->cname());

      jhost = Java_org_simgrid_msg_Host_getByName(env, cls, jname);

      env->ReleaseStringUTFChars(static_cast<jstring>(jname), host->cname());
    }

    env->SetObjectArrayElement(jtable, index, jhost);
  }
  xbt_dynar_free(&table);
  return jtable;
}

SG_END_DECL()
