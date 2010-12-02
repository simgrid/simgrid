/* Functions related to the java host instances.                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "jmsg.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"

jobject jhost_new_instance(JNIEnv * env)
{

  jclass cls = jxbt_get_class(env, "simgrid/msg/Host");
  jmethodID constructor = jxbt_get_jmethod(env, cls, "<init>", "()V");

  if (!constructor)
    return NULL;

  return (*env)->NewObject(env, cls, constructor);
}

jobject jhost_ref(JNIEnv * env, jobject jhost)
{
  return (*env)->NewGlobalRef(env, jhost);
}

void jhost_unref(JNIEnv * env, jobject jhost)
{
  (*env)->DeleteGlobalRef(env, jhost);
}

void jhost_bind(jobject jhost, m_host_t host, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Host", "bind", "J");

  if (!id)
    return;

  (*env)->SetLongField(env, jhost, id, (jlong) (long) (host));
}

m_host_t jhost_get_native(JNIEnv * env, jobject jhost)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Host", "bind", "J");

  if (!id)
    return NULL;

  return (m_host_t) (long) (*env)->GetLongField(env, jhost, id);
}

const char *jhost_get_name(jobject jhost, JNIEnv * env)
{
  m_host_t host = jhost_get_native(env, jhost);
  return (const char *) host->name;
}

void jhost_set_name(jobject jhost, jstring jname, JNIEnv * env)
{
  const char *name;
  m_host_t host = jhost_get_native(env, jhost);

  name = (*env)->GetStringUTFChars(env, jname, 0);

  if (host->name)
    free(host->name);

  host->name = xbt_strdup(name);
  (*env)->ReleaseStringUTFChars(env, jname, name);
}

jboolean jhost_is_valid(jobject jhost, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Host", "bind", "J");

  if (!id)
    return 0;

  if ((*env)->GetLongField(env, jhost, id)) {
    return JNI_TRUE;
  } else {
    return JNI_FALSE;
  }
}
