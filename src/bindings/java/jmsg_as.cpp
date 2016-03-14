/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "xbt/dict.h"
#include "simgrid/msg.h"
#include "jmsg_as.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"
#include "jmsg.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

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

void jas_bind(jobject jas, msg_as_t as, JNIEnv * env) {
  env->SetLongField(jas, jas_field_As_bind, (jlong) (uintptr_t) (as));
}

msg_as_t jas_get_native(JNIEnv * env, jobject jas) {
  return (msg_as_t) (uintptr_t) env->GetLongField(jas, jas_field_As_bind);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_As_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_As = env->FindClass("org/simgrid/msg/As");
  jas_method_As_constructor = env->GetMethodID(class_As, "<init>", "()V");
  jas_field_As_bind = jxbt_get_jfield(env,class_As, "bind", "J");
  if (!class_As || !jas_method_As_constructor || !jas_field_As_bind) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getName(JNIEnv * env, jobject jas) {
  msg_as_t as = jas_get_native(env, jas);
  return env->NewStringUTF(MSG_environment_as_get_name(as));
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getSons(JNIEnv * env, jobject jas) {
  int index = 0;
  jobjectArray jtable;
  jobject tmp_jas;
  msg_as_t tmp_as;
  msg_as_t self_as = jas_get_native(env, jas);
  
  xbt_dict_t dict = MSG_environment_as_get_routing_sons(self_as);
  int count = xbt_dict_length(dict);
  jclass cls = env->FindClass("org/simgrid/msg/As");

  if (!cls) {
    return NULL;
  }

  jtable = env->NewObjectArray((jsize) count, cls, NULL);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return NULL;
  }

  xbt_dict_cursor_t cursor=NULL;
  char *key;

  xbt_dict_foreach(dict,cursor,key,tmp_as) {
    tmp_jas = jas_new_instance(env);
    if (!tmp_jas) {
      jxbt_throw_jni(env, "java As instantiation failed");
      return NULL;
    }
    tmp_jas = jas_ref(env, tmp_jas);
    if (!tmp_jas) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return NULL;
    }
    jas_bind(tmp_jas, tmp_as, env);

    env->SetObjectArrayElement(jtable, index, tmp_jas);
    index++;
  }
  return jtable;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_As_getProperty(JNIEnv *env, jobject jas, jobject jname) {
  msg_as_t as = jas_get_native(env, jas);

  if (!as) {
    jxbt_throw_notbound(env, "as", jas);
    return NULL;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);

  const char *property = MSG_environment_as_get_property_value(as, name);
  if (!property) {
    return NULL;
  }

  jobject jproperty = env->NewStringUTF(property);

  env->ReleaseStringUTFChars((jstring) jname, name);

  return jproperty;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_As_getHosts(JNIEnv * env, jobject jas)
{
  int index;
  jobjectArray jtable;
  jobject jhost;
  jstring jname;
  msg_host_t host;
  msg_as_t as = jas_get_native(env, jas);

  xbt_dynar_t table =  MSG_environment_as_get_hosts(as);
  int count = xbt_dynar_length(table);

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");

  if (!cls) {
    return NULL;
  }

  jtable = env->NewObjectArray((jsize) count, cls, NULL);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return NULL;
  }

  for (index = 0; index < count; index++) {
    host = xbt_dynar_get_as(table,index,msg_host_t);

    jhost = (jobject) host->extension(JAVA_HOST_LEVEL);
    if (!jhost) {
      jname = env->NewStringUTF(MSG_host_get_name(host));

      jhost = Java_org_simgrid_msg_Host_getByName(env, cls, jname);

      /* FIXME: leak of jname ? */
    }

    env->SetObjectArrayElement(jtable, index, jhost);
  }
  xbt_dynar_free(&table);
  return jtable;
}
