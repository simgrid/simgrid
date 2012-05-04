/* Functions related to the java host instances.                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "jmsg.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

jobject jhost_new_instance(JNIEnv * env) {

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");
  jmethodID constructor = jxbt_get_jmethod(env, cls, "<init>", "()V");

  if (!constructor)
    return NULL;

  return (*env)->NewObject(env, cls, constructor);
}

jobject jhost_ref(JNIEnv * env, jobject jhost) {
  return (*env)->NewGlobalRef(env, jhost);
}

void jhost_unref(JNIEnv * env, jobject jhost) {
  (*env)->DeleteGlobalRef(env, jhost);
}

void jhost_bind(jobject jhost, m_host_t host, JNIEnv * env) {
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Host", "bind", "J");

  if (!id)
    return;

  (*env)->SetLongField(env, jhost, id, (jlong) (long) (host));
}

m_host_t jhost_get_native(JNIEnv * env, jobject jhost) {
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Host", "bind", "J");

  if (!id)
    return NULL;

  return (m_host_t) (long) (*env)->GetLongField(env, jhost, id);
}

const char *jhost_get_name(jobject jhost, JNIEnv * env) {
  m_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_name(host);
}

jboolean jhost_is_valid(jobject jhost, JNIEnv * env) {
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Host", "bind", "J");

  if (!id)
    return 0;

  if ((*env)->GetLongField(env, jhost, id)) {
    return JNI_TRUE;
  } else {
    return JNI_FALSE;
  }
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Host_getByName(JNIEnv * env, jclass cls,
                                         jstring jname) {
  m_host_t host;                /* native host                                          */
  jobject jhost;                /* global reference to the java host instance returned  */

  /* get the C string from the java string */
  const char *name = (*env)->GetStringUTFChars(env, jname, 0);
  if (name == NULL) {
  	jxbt_throw_null(env,bprintf("No host can have a null name"));
  	return NULL;
  }
  XBT_DEBUG("Looking for host '%s'",name);
  /* get the host by name       (the hosts are created during the grid resolution) */
  host = MSG_get_host_by_name(name);
  XBT_DEBUG("MSG gave %p as native host", host);

  if (!host) {                  /* invalid name */
    jxbt_throw_host_not_found(env, name);
    (*env)->ReleaseStringUTFChars(env, jname, name);
    return NULL;
  }
  (*env)->ReleaseStringUTFChars(env, jname, name);

  if (!MSG_host_get_data(host)) {       /* native host not associated yet with java host */

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

    /* bind the java host and the native host */
    jhost_bind(jhost, host, env);

    /* the native host data field is set with the global reference to the
     * java host returned by this function
     */
    MSG_host_set_data(host, (void *) jhost);
  }

  /* return the global reference to the java host instance */
  return (jobject) MSG_host_get_data(host);
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Host_currentHost(JNIEnv * env, jclass cls) {
  jobject jhost;

  m_host_t host = MSG_host_self();

  if (!MSG_host_get_data(host)) {
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

    /* Bind & store it */
    jhost_bind(jhost, host, env);
    MSG_host_set_data(host, (void *) jhost);
  } else {
    jhost = (jobject) MSG_host_get_data(host);
  }

  return jhost;
}

JNIEXPORT jstring JNICALL
Java_org_simgrid_msg_Host_getName(JNIEnv * env,
                                  jobject jhost) {
  m_host_t host = jhost_get_native(env, jhost);
  const char* name;

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return NULL;
  }

  name = MSG_host_get_name(host);
  if (!name)
	  xbt_die("This host has no name...");

  return (*env)->NewStringUTF(env, name);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_Host_getCount(JNIEnv * env, jclass cls) {
  xbt_dynar_t hosts =  MSG_hosts_as_dynar();
  int nb_host = xbt_dynar_length(hosts);
  xbt_dynar_free(&hosts);
  return (jint) nb_host;
}

JNIEXPORT jdouble JNICALL
Java_org_simgrid_msg_Host_getSpeed(JNIEnv * env,
                                        jobject jhost) {
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_get_host_speed(host);
}
JNIEXPORT jint JNICALL
Java_org_simgrid_msg_Host_getLoad(JNIEnv * env, jobject jhost) {
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jint) MSG_get_host_msgload(host);
}
JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_Host_isAvail(JNIEnv * env, jobject jhost) {
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return (jboolean) MSG_host_is_avail(host);
}
