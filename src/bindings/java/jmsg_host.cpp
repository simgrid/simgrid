/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "simgrid/msg.h"
#include "jmsg.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"
#include "jmsg_storage.h"
#include <surf/surf_routing.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

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

const char *jhost_get_name(jobject jhost, JNIEnv * env) {
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_name(host);
}

jboolean jhost_is_valid(jobject jhost, JNIEnv * env) {
  if (env->GetLongField(jhost, jhost_field_Host_bind)) {
    return JNI_TRUE;
  } else {
    return JNI_FALSE;
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_Host = env->FindClass("org/simgrid/msg/Host");
  jhost_method_Host_constructor = env->GetMethodID(class_Host, "<init>", "()V");
  jhost_field_Host_bind = jxbt_get_jfield(env,class_Host, "bind", "J");
  jhost_field_Host_name = jxbt_get_jfield(env, class_Host, "name", "Ljava/lang/String;");
  if (!class_Host || !jhost_field_Host_name || !jhost_method_Host_constructor || !jhost_field_Host_bind) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getByName(JNIEnv * env, jclass cls, jstring jname) {
  msg_host_t host;                /* native host                                          */
  jobject jhost;                /* global reference to the java host instance returned  */

  /* get the C string from the java string */
  if (jname == NULL) {
    jxbt_throw_null(env,bprintf("No host can have a null name"));
    return NULL;
  }
  const char *name = env->GetStringUTFChars(jname, 0);
  /* get the host by name       (the hosts are created during the grid resolution) */
  host = MSG_host_by_name(name);

  if (!host) {                  /* invalid name */
    jxbt_throw_host_not_found(env, name);
    env->ReleaseStringUTFChars(jname, name);
    return NULL;
  }
  env->ReleaseStringUTFChars(jname, name);

  if (!host->extension(JAVA_HOST_LEVEL)) {       /* native host not associated yet with java host */
    /* Instantiate a new java host */
    jhost = jhost_new_instance(env);

    if (!jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return NULL;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (!jhost) {
      jxbt_throw_jni(env, "new global ref allocation failed");
      return NULL;
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

  if (!host->extension(JAVA_HOST_LEVEL)) {
    /* the native host not yet associated with the java host instance */

    /* instanciate a new java host instance */
    jhost = jhost_new_instance(env);

    if (!jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return NULL;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (!jhost) {
      jxbt_throw_jni(env, "global ref allocation failed");
      return NULL;
    }
    /* Sets the host name */
    const char *name = MSG_host_get_name(host);
    jobject jname = env->NewStringUTF(name);
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
  MSG_host_off(host); 
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Host_getCount(JNIEnv * env, jclass cls) {
  xbt_dynar_t hosts =  MSG_hosts_as_dynar();
  int nb_host = xbt_dynar_length(hosts);
  xbt_dynar_free(&hosts);
  return (jint) nb_host;
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getSpeed(JNIEnv * env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_host_get_speed(host);
}

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getCoreNumber(JNIEnv * env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_host_get_core_number(host);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Host_getProperty(JNIEnv *env, jobject jhost, jobject jname) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return NULL;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);

  const char *property = MSG_host_get_property_value(host, name);
  if (!property) {
    return NULL;
  }

  jobject jproperty = env->NewStringUTF(property);

  env->ReleaseStringUTFChars((jstring) jname, name);

  return jproperty;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Host_setProperty(JNIEnv *env, jobject jhost, jobject jname, jobject jvalue) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }
  const char *name = env->GetStringUTFChars((jstring) jname, 0);
  const char *value_java = env->GetStringUTFChars((jstring) jvalue, 0);
  char *value = xbt_strdup(value_java);

  MSG_host_set_property_value(host, name, value, xbt_free_f);

  env->ReleaseStringUTFChars((jstring) jvalue, value_java);
  env->ReleaseStringUTFChars((jstring) jname, name);
}

JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Host_isOn(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
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

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  int index = 0;
  jobjectArray jtable;
  xbt_dict_t dict =  MSG_host_get_mounted_storage_list(host);
  int count = xbt_dict_length(dict);
  jclass cls = env->FindClass("org/simgrid/msg/Storage");

  jtable = env->NewObjectArray((jsize) count, cls, NULL);

  if (!jtable) {
   jxbt_throw_jni(env, "Storages table allocation failed");
   return NULL;
  }

  xbt_dict_cursor_t cursor=NULL;
  const char *mount_name, *storage_name;

  xbt_dict_foreach(dict,cursor,mount_name,storage_name) {
    jname = env->NewStringUTF(storage_name);
    jstorage = Java_org_simgrid_msg_Storage_getByName(env,cls,jname);
    env->SetObjectArrayElement(jtable, index, jstorage);
    index++;
  }
  xbt_dict_free(&dict);
  return jtable;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getAttachedStorage(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }
  jobjectArray jtable;

  xbt_dynar_t dyn = MSG_host_get_attached_storage_list(host);
  int count = xbt_dynar_length(dyn);
  jclass cls = jxbt_get_class(env, "java/lang/String");
  jtable = env->NewObjectArray((jsize) count, cls, NULL);
  int index;
  char *storage_name;
  jstring jstorage_name;
  for (index = 0; index < count; index++) {
    storage_name = xbt_dynar_get_as(dyn,index,char*);
    jstorage_name = env->NewStringUTF(storage_name);
    env->SetObjectArrayElement(jtable, index, jstorage_name);
  }

  return jtable;
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_getStorageContent(JNIEnv * env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }
  return (jobjectArray)MSG_host_get_storage_content(host);
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_Host_all(JNIEnv * env, jclass cls_arg)
{
  int index;
  jobjectArray jtable;
  jobject jhost;
  jstring jname;
  msg_host_t host;

  xbt_dynar_t table =  MSG_hosts_as_dynar();
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

      jhost = Java_org_simgrid_msg_Host_getByName(env, cls_arg, jname);
      /* FIXME: leak of jname ? */
    }

    env->SetObjectArrayElement(jtable, index, jhost);
  }
  xbt_dynar_free(&table);
  return jtable;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Host_setAsyncMailbox(JNIEnv * env, jclass cls_arg, jobject jname)
{
  const char *name = env->GetStringUTFChars((jstring) jname, 0);
  MSG_mailbox_set_async(name);
  env->ReleaseStringUTFChars((jstring) jname, name);
}

#include "simgrid/plugins/energy.h"
JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Host_getConsumedEnergy (JNIEnv *env, jobject jhost)
{
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return MSG_host_get_consumed_energy(host);
}
