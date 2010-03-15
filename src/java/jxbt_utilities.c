/*
 * $Id$
 *
 * Various JNI helper functions
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 */

#include <stdlib.h>             /* abort */
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/str.h"
#include "jxbt_utilities.h"

/* *********** */
/* JNI GETTERS */
/* *********** */

jclass jxbt_get_class(JNIEnv * env, const char *name)
{
  jclass cls = (*env)->FindClass(env, name);

  if (!cls) {
    char *m = bprintf("Class %s not found", name);
    jxbt_throw_jni(env, m);
    free(m);
    return NULL;
  }

  return cls;
}

jmethodID jxbt_get_jmethod(JNIEnv * env, jclass cls,
                           const char *name, const char *signature)
{
  jmethodID id;

  if (!cls)
    return 0;
  id = (*env)->GetMethodID(env, cls, name, signature);

  if (!id) {

    jmethodID tostr_id =
      (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");
    jstring jclassname =
      (jstring) (*env)->CallObjectMethod(env, cls, tostr_id, NULL);
    const char *classname = (*env)->GetStringUTFChars(env, jclassname, 0);

    char *m =
      bprintf("Cannot find method %s(%s) in %s", name, signature, classname);

    (*env)->ReleaseStringUTFChars(env, jclassname, classname);

    jxbt_throw_jni(env, m);

    free(m);
    return 0;
  }

  return id;
}

jmethodID jxbt_get_static_jmethod(JNIEnv * env, jclass cls,
                                  const char *name, const char *signature)
{
  jmethodID id;

  if (!cls)
    return 0;
  id = (*env)->GetStaticMethodID(env, cls, name, signature);

  if (!id) {

    jmethodID tostr_id =
      (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");
    jstring jclassname =
      (jstring) (*env)->CallObjectMethod(env, cls, tostr_id, NULL);
    const char *classname = (*env)->GetStringUTFChars(env, jclassname, 0);

    char *m =
      bprintf("Cannot find static method %s(%s) in %s", name, signature,
              classname);

    (*env)->ReleaseStringUTFChars(env, jclassname, classname);

    jxbt_throw_jni(env, m);

    free(m);
    return 0;
  }

  return id;
}

jmethodID jxbt_get_static_smethod(JNIEnv * env, const char *classname,
                                  const char *name, const char *signature)
{

  jclass cls;
  jmethodID id;
  cls = jxbt_get_class(env, classname);

  if (!cls)
    return 0;

  id = (*env)->GetStaticMethodID(env, cls, name, signature);

  if (!id) {
    char *m =
      bprintf("Cannot find static method %s(%s) in %s", name, signature,
              classname);

    jxbt_throw_jni(env, m);

    free(m);
    return 0;
  }
  return id;
}

jmethodID jxbt_get_smethod(JNIEnv * env, const char *classname,
                           const char *name, const char *signature)
{

  jclass cls;
  jmethodID id;
  cls = jxbt_get_class(env, classname);

  if (!cls)
    return 0;

  id = (*env)->GetMethodID(env, cls, name, signature);

  if (!id) {
    char *m =
      bprintf("Cannot find method %s(%s) in %s", name, signature, classname);

    jxbt_throw_jni(env, m);

    free(m);
    return 0;
  }
  return id;
}

jfieldID jxbt_get_jfield(JNIEnv * env, jclass cls,
                         const char *name, const char *signature)
{
  jfieldID id;

  if (!cls)
    return 0;

  id = (*env)->GetFieldID(env, cls, name, signature);

  if (!id) {
    jmethodID getname_id =
      (*env)->GetMethodID(env, cls, "getName", "()Ljava/lang/String;");
    jstring jclassname =
      (jstring) (*env)->CallObjectMethod(env, cls, getname_id, NULL);
    const char *classname = (*env)->GetStringUTFChars(env, jclassname, 0);
    char *m =
      bprintf("Cannot find field %s %s in %s", signature, name, classname);

    (*env)->ReleaseStringUTFChars(env, jclassname, classname);

    jxbt_throw_jni(env, m);

    free(m);
    return 0;
  }

  return id;
}

jfieldID jxbt_get_sfield(JNIEnv * env, const char *classname,
                         const char *name, const char *signature)
{
  jclass cls = jxbt_get_class(env, classname);
  jfieldID id;

  if (!cls)
    return 0;

  id = (*env)->GetFieldID(env, cls, name, signature);

  if (!id) {
    char *m =
      bprintf("Cannot find field %s %s in %s", signature, name, classname);

    jxbt_throw_jni(env, m);

    free(m);
    return 0;
  }

  return id;
}

/* ***************** */
/* EXCEPTION RAISING */
/* ***************** */
static void jxbt_throw_by_name(JNIEnv * env, const char *name, char *msg)
{
  jclass cls = (*env)->FindClass(env, name);

  xbt_assert2(cls, "%s (Plus severe error: class %s not found)\n", msg, name);

  (*env)->ThrowNew(env, cls, msg);

  free(msg);
}


/* Errors in MSG */
void jxbt_throw_jni(JNIEnv * env, const char *msg)
{
  jxbt_throw_by_name(env,
                     "simgrid/msg/JniException",
                     bprintf("Internal or JNI error: %s", msg));
}

void jxbt_throw_notbound(JNIEnv * env, const char *kind, void *pointer)
{
  jxbt_throw_by_name(env,
                     "simgrid/msg/JniException",
                     bprintf("Internal error: %s %p not bound", kind,
                             pointer));
}

void jxbt_throw_native(JNIEnv * env, char *msg)
{
  jxbt_throw_by_name(env, "simgrid/msg/NativeException", msg);
}

/* *** */
void jxbt_throw_null(JNIEnv * env, char *msg)
{
  jxbt_throw_by_name(env, "java/lang/NullPointerException", msg);
}

/* Errors on user side */
void jxbt_throw_illegal(JNIEnv * env, char *msg)
{
  jxbt_throw_by_name(env, "java/lang/IllegalArgumentException", msg);
}

void jxbt_throw_host_not_found(JNIEnv * env, const char *invalid_name)
{
  jxbt_throw_by_name(env,
                     "simgrid/msg/HostNotFoundException",
                     bprintf("No such host: %s", invalid_name));
}

void jxbt_throw_process_not_found(JNIEnv * env, const char *invalid_name)
{
  jxbt_throw_by_name(env,
                     "simgrid/msg/ProcessNotFoundException",
                     bprintf("No such process: %s", invalid_name));
}

// tranfert failure
void jxbt_throw_transfer_failure(JNIEnv *env,const char *task_name,const char *alias)
{
  
  jxbt_throw_by_name(env,
		     "simgrid/msg/TransferFailureException",
		     bprintf("There has been a problem during your task transfer (task :%s / alias :%s)",task_name,alias));
  
}

// host failure Exception
void jxbt_throw_host_failure(JNIEnv *env,const char *task_name,const char *alias)
{
  
 jxbt_throw_by_name(env,
		    "simgrid/msg/HostFailureException",
		    bprintf("Host Failure while sending (task :%s / alias %s) : The host on which you are running has just been rebooted",task_name,alias));
  
}