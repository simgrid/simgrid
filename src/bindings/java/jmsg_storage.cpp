/* Java bindings of the Storage API.                                        */

/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "simgrid/plugins/file_system.h"

#include "include/xbt/signal.hpp"
#include "jmsg.hpp"
#include "jmsg_storage.h"
#include "jxbt_utilities.hpp"
#include "simgrid/s4u/Storage.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

static jmethodID jstorage_method_Storage_constructor;
static jfieldID jstorage_field_Storage_bind;
static jfieldID jstorage_field_Storage_name;

jobject jstorage_new_instance(JNIEnv * env) {
  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Storage");
  return env->NewObject(cls, jstorage_method_Storage_constructor);
}

msg_storage_t jstorage_get_native(JNIEnv * env, jobject jstorage) {
  return (msg_storage_t) (uintptr_t) env->GetLongField(jstorage, jstorage_field_Storage_bind);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Storage_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_Storage = env->FindClass("org/simgrid/msg/Storage");
  xbt_assert(class_Storage, "Native initialization of msg/Storage failed. Please report that bug");
  jstorage_method_Storage_constructor = env->GetMethodID(class_Storage, "<init>", "()V");
  jstorage_field_Storage_bind = jxbt_get_jfield(env,class_Storage, "bind", "J");
  jstorage_field_Storage_name = jxbt_get_jfield(env, class_Storage, "name", "Ljava/lang/String;");
  xbt_assert(jstorage_field_Storage_name && jstorage_method_Storage_constructor && jstorage_field_Storage_bind,
             "Native initialization of msg/Storage failed. Please report that bug");
}

void jstorage_bind(jobject jstorage, msg_storage_t storage, JNIEnv * env) {
  env->SetLongField(jstorage, jstorage_field_Storage_bind, (jlong) (uintptr_t) (storage));
}

jobject jstorage_ref(JNIEnv * env, jobject jstorage) {
  return env->NewGlobalRef(jstorage);
}

void jstorage_unref(JNIEnv * env, jobject jstorage) {
  env->DeleteGlobalRef(jstorage);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getByName(JNIEnv * env, jclass cls, jstring jname) {
  msg_storage_t storage;
  jobject jstorage = nullptr;

  /* get the C string from the java string */
  if (jname == nullptr) {
    jxbt_throw_null(env, "No host can have a null name");
    return nullptr;
  }
  const char *name = env->GetStringUTFChars(jname, 0);
  storage = MSG_storage_get_by_name(name);

  if (not storage) { /* invalid name */
    jxbt_throw_storage_not_found(env, name);
    env->ReleaseStringUTFChars(jname, name);
    return nullptr;
  }
  env->ReleaseStringUTFChars(jname, name);

  if (java_storage_map.find(storage) == java_storage_map.end()) {
    /* Instantiate a new java storage */
    jstorage = jstorage_new_instance(env);

    if (not jstorage) {
      jxbt_throw_jni(env, "java storage instantiation failed");
      return nullptr;
    }

    /* get a global reference to the newly created storage */
    jstorage = jstorage_ref(env, jstorage);

    if (not jstorage) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return nullptr;
    }
    /* Sets the java storage name */
    env->SetObjectField(jstorage, jstorage_field_Storage_name, jname);
    /* bind the java storage and the native storage */
    jstorage_bind(jstorage, storage, env);

    /* the native storage data field is set with the global reference to the
     * java storage returned by this function
     */
    java_storage_map.insert({storage, jstorage});
  } else
    jstorage = java_storage_map.at(storage);

  /* return the global reference to the java storage instance */
  return (jobject)jstorage;
}

JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getSize(JNIEnv * env,jobject jstorage) {
  const_sg_storage_t storage = jstorage_get_native(env, jstorage);

  if (not storage) {
    jxbt_throw_notbound(env, "storage", jstorage);
    return -1;
  }

  return (jlong) MSG_storage_get_size(storage);
}

JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getFreeSize(JNIEnv * env,jobject jstorage) {
  const_sg_storage_t storage = jstorage_get_native(env, jstorage);

  if (not storage) {
    jxbt_throw_notbound(env, "storage", jstorage);
    return -1;
  }

  return (jlong) MSG_storage_get_free_size(storage);
}

JNIEXPORT jlong JNICALL Java_org_simgrid_msg_Storage_getUsedSize(JNIEnv * env,jobject jstorage) {
  const_sg_storage_t storage = jstorage_get_native(env, jstorage);

  if (not storage) {
    jxbt_throw_notbound(env, "storage", jstorage);
    return -1;
  }

  return (jlong) MSG_storage_get_used_size(storage);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getProperty(JNIEnv *env, jobject jstorage, jobject jname) {
  const_sg_storage_t storage = jstorage_get_native(env, jstorage);

  if (not storage) {
    jxbt_throw_notbound(env, "storage", jstorage);
    return nullptr;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);

  const char *property = MSG_storage_get_property_value(storage, name);
  if (not property) {
    return nullptr;
  }
  jobject jproperty = env->NewStringUTF(property);

  env->ReleaseStringUTFChars((jstring) jname, name);

  return jproperty;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Storage_setProperty(JNIEnv *env, jobject jstorage, jobject jname, jobject jvalue) {
  msg_storage_t storage = jstorage_get_native(env, jstorage);

  if (not storage) {
    jxbt_throw_notbound(env, "storage", jstorage);
    return;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);
  const char *value_java = env->GetStringUTFChars((jstring) jvalue, 0);

  storage->set_property(name, std::string(value_java));

  env->ReleaseStringUTFChars((jstring) jvalue, value_java);
  env->ReleaseStringUTFChars((jstring) jname, name);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Storage_getHost(JNIEnv * env,jobject jstorage) {
  const_sg_storage_t storage = jstorage_get_native(env, jstorage);

  if (not storage) {
    jxbt_throw_notbound(env, "storage", jstorage);
    return nullptr;
  }
  const char *host_name = MSG_storage_get_host(storage);
  if (not host_name) {
    return nullptr;
  }
  jobject jhost_name = env->NewStringUTF(host_name);

  return jhost_name;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Storage_all(JNIEnv * env, jclass cls_arg)
{
  int index;
  jobjectArray jtable;
  jobject jstorage;
  jstring jname;
  msg_storage_t storage;

  xbt_dynar_t table =  MSG_storages_as_dynar();
  int count = xbt_dynar_length(table);

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Storage");

  if (not cls) {
    return nullptr;
  }

  jtable = env->NewObjectArray((jsize) count, cls, nullptr);

  if (not jtable) {
    jxbt_throw_jni(env, "Storages table allocation failed");
    return nullptr;
  }

  for (index = 0; index < count; index++) {
    storage = xbt_dynar_get_as(table,index,msg_storage_t);
    if (java_storage_map.find(storage) != java_storage_map.end()) {
      jstorage = java_storage_map.at(storage);
    } else {
      jname = env->NewStringUTF(MSG_storage_get_name(storage));
      jstorage = Java_org_simgrid_msg_Storage_getByName(env, cls_arg, jname);
    }

    env->SetObjectArrayElement(jtable, index, jstorage);
  }
  xbt_dynar_free(&table);
  return jtable;
}
