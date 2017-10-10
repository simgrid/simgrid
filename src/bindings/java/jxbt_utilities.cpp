/* Various JNI helper functions                                             */

/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jxbt_utilities.hpp"
#include "xbt/sysdep.h"

#include <cstdlib> /* abort */

extern "C" {

jclass jxbt_get_class(JNIEnv * env, const char *name)
{
  jclass cls = env->FindClass(name);

  if (not cls) {
    jxbt_throw_jni(env, std::string("Class ") + name + " not found");
    return nullptr;
  }

  return cls;
}

jmethodID jxbt_get_jmethod(JNIEnv * env, jclass cls, const char *name, const char *signature)
{
  jmethodID id;

  if (not cls)
    return 0;
  id = env->GetMethodID(cls, name, signature);

  if (not id) {

    jmethodID tostr_id = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    jstring jclassname = (jstring) env->CallObjectMethod(cls, tostr_id, nullptr);
    const char *classname = env->GetStringUTFChars(jclassname, 0);

    env->ReleaseStringUTFChars(jclassname, classname);

    jxbt_throw_jni(env, std::string("Cannot find method") + name + "(" + signature + ") in " + classname);
    return 0;
  }

  return id;
}

jmethodID jxbt_get_static_jmethod(JNIEnv * env, jclass cls, const char *name, const char *signature)
{
  jmethodID id;

  if (not cls)
    return 0;
  id = env->GetStaticMethodID(cls, name, signature);

  if (not id) {
    jmethodID tostr_id = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    jstring jclassname = (jstring) env->CallObjectMethod(cls, tostr_id, nullptr);
    const char *classname = env->GetStringUTFChars(jclassname, 0);

    env->ReleaseStringUTFChars(jclassname, classname);

    jxbt_throw_jni(env, std::string("Cannot find static method") + name + "(" + signature + ") in " + classname);
    return 0;
  }

  return id;
}

jmethodID jxbt_get_static_smethod(JNIEnv * env, const char *classname, const char *name, const char *signature)
{
  jclass cls;
  jmethodID id;
  cls = jxbt_get_class(env, classname);

  if (not cls)
    return 0;

  id = env->GetStaticMethodID(cls, name, signature);

  if (not id) {
    jxbt_throw_jni(env, std::string("Cannot find static method") + name + "(" + signature + ") in " + classname);
    return 0;
  }
  return id;
}

jmethodID jxbt_get_smethod(JNIEnv * env, const char *classname, const char *name, const char *signature)
{
  jclass cls;
  jmethodID id;
  cls = jxbt_get_class(env, classname);

  if (not cls)
    return 0;

  id = env->GetMethodID(cls, name, signature);

  if (not id) {
    jxbt_throw_jni(env, std::string("Cannot find method") + name + "(" + signature + ") in " + classname);
    return 0;
  }
  return id;
}

jfieldID jxbt_get_jfield(JNIEnv * env, jclass cls, const char *name, const char *signature)
{
  jfieldID id;

  if (not cls)
    return 0;

  id = env->GetFieldID(cls, name, signature);

  if (not id) {
    jmethodID getname_id = env->GetMethodID(cls, "getName", "()Ljava/lang/String;");
    jstring jclassname = (jstring) env->CallObjectMethod(cls, getname_id, nullptr);
    const char *classname = env->GetStringUTFChars(jclassname, 0);

    env->ReleaseStringUTFChars(jclassname, classname);

    jxbt_throw_jni(env, std::string("Cannot find field") + signature + " " + name + " in " + classname);

    return 0;
  }

  return id;
}

jfieldID jxbt_get_sfield(JNIEnv * env, const char *classname, const char *name, const char *signature)
{
  jclass cls = jxbt_get_class(env, classname);
  jfieldID id;

  if (not cls)
    return 0;

  id = env->GetFieldID(cls, name, signature);

  if (not id) {
    jxbt_throw_jni(env, std::string("Cannot find field") + signature + " " + name + " in " + classname);
    return 0;
  }

  return id;
}

void jxbt_throw_by_name(JNIEnv* env, const char* name, std::string msg)
{
  jclass cls = env->FindClass(name);

  xbt_assert(cls, "%s (Plus severe error: class %s not found)\n", msg.c_str(), name);

  env->ThrowNew(cls, msg.c_str());
}

void jxbt_throw_jni(JNIEnv* env, std::string msg)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/JniException", "Internal or JNI error: " + msg);
}

void jxbt_throw_notbound(JNIEnv* env, std::string kind, void* pointer)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/JniException",
                     "Internal error: " + kind + " " + static_cast<const char*>(pointer) + " not bound");
}

void jxbt_throw_null(JNIEnv* env, std::string msg)
{
  jxbt_throw_by_name(env, "java/lang/NullPointerException", msg);
}

void jxbt_throw_illegal(JNIEnv* env, std::string msg)
{
  jxbt_throw_by_name(env, "java/lang/IllegalArgumentException", msg);
}

void jxbt_throw_host_not_found(JNIEnv* env, std::string invalid_name)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/HostNotFoundException", "No such host: " + invalid_name);
}

void jxbt_throw_storage_not_found(JNIEnv* env, std::string invalid_name)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/StorageNotFoundException", "No such storage: " + invalid_name);
}

void jxbt_throw_process_not_found(JNIEnv* env, std::string invalid_name)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/ProcessNotFoundException", "No such process: " + invalid_name);
}

void jxbt_throw_transfer_failure(JNIEnv* env, std::string details)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/TransferFailureException", details);
}

void jxbt_throw_host_failure(JNIEnv* env, std::string details)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/HostFailureException", "Host Failure " + details);
}

void jxbt_throw_time_out_failure(JNIEnv* env, std::string details)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/TimeoutException", details);
}

void jxbt_throw_task_cancelled(JNIEnv* env, std::string details)
{
  jxbt_throw_by_name(env, "org/simgrid/msg/TaskCancelledException", details);
}
}
