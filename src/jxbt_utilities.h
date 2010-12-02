/* Various JNI helper functions                                             */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JXBT_UTILITY_H
#define JXBT_UTILITY_H

#include <jni.h>

/* *********** */
/* JNI GETTERS */
/* *********** */

/* Search a class and throw an exception if not found */
jclass jxbt_get_class(JNIEnv * env, const char *name);

/* Search a method in a class and throw an exception if not found
   (it's ok to to pass a NULL class: it's a noop) */
jmethodID jxbt_get_jmethod(JNIEnv * env, jclass class,
                           const char *name, const char *signature);

/* Like the jxbt_get_class() but get a static method */
jmethodID jxbt_get_static_jmethod(JNIEnv * env, jclass cls,
                                  const char *name, const char *signature);

/* Search a field in a class and throw an exception if not found
   (it's ok to to pass a NULL class: it's a noop) */
jfieldID jxbt_get_jfield(JNIEnv * env, jclass class,
                         const char *name, const char *signature);


/* Search a method in a class and throw an exception if not found
   (it's ok to to pass a NULL class: it's a noop) */
jmethodID jxbt_get_smethod(JNIEnv * env, const char *classname,
                           const char *name, const char *signature);

/* Like the jxbt_get_smethod() but get a static method */
jmethodID jxbt_get_static_smethod(JNIEnv * env, const char *classname,
                                  const char *name, const char *signature);

/* Search a field in a class and throw an exception if not found
   (it's ok to to pass a NULL class: it's a noop) */
jfieldID jxbt_get_sfield(JNIEnv * env, const char *classname,
                         const char *name, const char *signature);


/* ***************** */
/* EXCEPTION RAISING */
/* ***************** */

#define jxbt_check_res(fun, res, allowed_exceptions, detail) do {\
    if (res != MSG_OK && res | allowed_exceptions) { \
      xbt_die(bprintf("%s failed with error code %d, which is not an allowed exception. Please fix me.",fun,res)); \
    } else if (res == MSG_HOST_FAILURE) { \
      jxbt_throw_host_failure(env, detail); \
    } else if (res == MSG_TRANSFER_FAILURE) { \
      jxbt_throw_transfer_failure(env,detail); \
    } else if (res == MSG_TIMEOUT) { \
      jxbt_throw_time_out_failure(env,detail); \
    } else if (res == MSG_TASK_CANCELLED){ \
      jxbt_throw_task_cancelled(env,detail); \
   } } while (0)

/** Thrown on internal error of this layer, or on problem with JNI */
void jxbt_throw_jni(JNIEnv * env, const char *msg);
/** Thrown when using an object not bound to a native one where it should, or reverse (kinda JNI issue) */
void jxbt_throw_notbound(JNIEnv * env, const char *kind, void *pointer);

/** Thrown on error in native MSG code */
void jxbt_throw_native(JNIEnv * env, char *msg);

/** Thrown if NULL gets used */
void jxbt_throw_null(JNIEnv * env, char *msg);

/** Thrown on illegal arguments */
void jxbt_throw_illegal(JNIEnv * env, char *msg);
/** Thrown when looking for an host from name does not lead to anything */
void jxbt_throw_host_not_found(JNIEnv * env, const char *invalid_name);
/** Thrown when looking for an host from name does not lead to anything */
void jxbt_throw_process_not_found(JNIEnv * env, const char *invalid_name);
/** Thrown when a transfer failure accure while Sending task */
void jxbt_throw_transfer_failure(JNIEnv * env, char *detail);
/** Thrown when a host failure accures while Sending task*/
void jxbt_throw_host_failure(JNIEnv * env, char *details);
/** Thrown when a time out accures While Sending task */
void jxbt_throw_time_out_failure(JNIEnv * env, char *details);
/**Thrown when a task is canceled */
void jxbt_throw_task_cancelled(JNIEnv * env, char *details);

#endif                          /* ! JXBT_UTILITY_H */
