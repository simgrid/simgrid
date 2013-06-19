/* Functions related to the java host instances.                            */

/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/str.h"
#include "msg/msg.h"
#include "jmsg.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jmethodID jhost_method_Host_constructor;
static jfieldID jhost_field_Host_bind;
static jfieldID jhost_field_Host_name;


jobject jhost_new_instance(JNIEnv * env) {
  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");
  return (*env)->NewObject(env, cls, jhost_method_Host_constructor);
}

jobject jhost_ref(JNIEnv * env, jobject jhost) {
  return (*env)->NewGlobalRef(env, jhost);
}

void jhost_unref(JNIEnv * env, jobject jhost) {
  (*env)->DeleteGlobalRef(env, jhost);
}

void jhost_bind(jobject jhost, msg_host_t host, JNIEnv * env) {
  (*env)->SetLongField(env, jhost, jhost_field_Host_bind, (jlong) (long) (host));
}

msg_host_t jhost_get_native(JNIEnv * env, jobject jhost) {
  return (msg_host_t) (long) (*env)->GetLongField(env, jhost, jhost_field_Host_bind);
}

const char *jhost_get_name(jobject jhost, JNIEnv * env) {
  msg_host_t host = jhost_get_native(env, jhost);
  return MSG_host_get_name(host);
}

jboolean jhost_is_valid(jobject jhost, JNIEnv * env) {
  if ((*env)->GetLongField(env, jhost, jhost_field_Host_bind)) {
    return JNI_TRUE;
  } else {
    return JNI_FALSE;
  }
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Host_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_Host = (*env)->FindClass(env, "org/simgrid/msg/Host");
  jhost_method_Host_constructor = (*env)->GetMethodID(env, class_Host, "<init>", "()V");
  jhost_field_Host_bind = jxbt_get_jfield(env,class_Host, "bind", "J");
  jhost_field_Host_name = jxbt_get_jfield(env, class_Host, "name", "Ljava/lang/String;");
  if (!class_Host || !jhost_field_Host_name || !jhost_method_Host_constructor || !jhost_field_Host_bind) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}
JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Host_getByName(JNIEnv * env, jclass cls,
                                         jstring jname) {
  msg_host_t host;                /* native host                                          */
  jobject jhost;                /* global reference to the java host instance returned  */

  /* get the C string from the java string */
  const char *name = (*env)->GetStringUTFChars(env, jname, 0);
  if (name == NULL) {
  	jxbt_throw_null(env,bprintf("No host can have a null name"));
  	return NULL;
  }
  /* get the host by name       (the hosts are created during the grid resolution) */
  host = MSG_get_host_by_name(name);

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
    /* Sets the java host name */
    (*env)->SetObjectField(env, jhost, jhost_field_Host_name, jname);
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

  msg_host_t host = MSG_host_self();

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
    /* Sets the host name */
    const char *name = MSG_host_get_name(host);
    jobject jname = (*env)->NewStringUTF(env,name);
    (*env)->SetObjectField(env, jhost, jhost_field_Host_name, jname);
    /* Bind & store it */
    jhost_bind(jhost, host, env);
    MSG_host_set_data(host, (void *) jhost);
  } else {
    jhost = (jobject) MSG_host_get_data(host);
  }

  return jhost;
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
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_get_host_speed(host);
}

JNIEXPORT jdouble JNICALL
Java_org_simgrid_msg_Host_getCore(JNIEnv * env,
                                        jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jdouble) MSG_get_host_core(host);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_Host_getLoad(JNIEnv * env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jint) MSG_get_host_msgload(host);
}
JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Host_getProperty(JNIEnv *env, jobject jhost, jobject jname) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return NULL;
  }
  const char *name = (*env)->GetStringUTFChars(env, jname, 0);

  const char *property = MSG_host_get_property_value(host, name);
  if (!property) {
    return NULL;
  }

  jobject jproperty = (*env)->NewStringUTF(env, property);

  (*env)->ReleaseStringUTFChars(env, jname, name);

  return jproperty;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Host_setProperty(JNIEnv *env, jobject jhost, jobject jname, jobject jvalue) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }
  const char *name = (*env)->GetStringUTFChars(env, jname, 0);
  const char *value_java = (*env)->GetStringUTFChars(env, jvalue, 0);
  char *value = strdup(value_java);

  MSG_host_set_property_value(host,name,value,xbt_free);

  (*env)->ReleaseStringUTFChars(env, jvalue, value);
  (*env)->ReleaseStringUTFChars(env, jname, name);

}
JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_Host_isAvail(JNIEnv * env, jobject jhost) {
  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return 0;
  }

  return (jboolean) MSG_host_is_avail(host);
}

JNIEXPORT jobjectArray JNICALL
Java_org_simgrid_msg_Host_all(JNIEnv * env, jclass cls_arg)
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

  jtable = (*env)->NewObjectArray(env, (jsize) count, cls, NULL);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return NULL;
  }

  for (index = 0; index < count; index++) {
    host = xbt_dynar_get_as(table,index,msg_host_t);
    jhost = (jobject) (MSG_host_get_data(host));

    if (!jhost) {
      jname = (*env)->NewStringUTF(env, MSG_host_get_name(host));

      jhost =
      		Java_org_simgrid_msg_Host_getByName(env, cls_arg, jname);
      /* FIXME: leak of jname ? */
    }

    (*env)->SetObjectArrayElement(env, jtable, index, jhost);
  }
  xbt_dynar_free(&table);
  return jtable;
}

JNIEXPORT void JNICALL 
Java_org_simgrid_msg_Host_setAsyncMailbox(JNIEnv * env, jclass cls_arg, jobject jname){

  const char *name = (*env)->GetStringUTFChars(env, jname, 0);
  MSG_mailbox_set_async(name);
  (*env)->ReleaseStringUTFChars(env, jname, name);

}
