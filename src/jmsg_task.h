/* Functions related to the java task instances.                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JTASK_H
#define MSG_JTASK_H

#include <jni.h>
#include "msg/msg.h"

/**
 * This function returns a global reference to the  java task instance 
 * specified by the parameter jtask.
 *
 * @param jtask			The original java task instance.
 * @param env			The environment of the current thread.
 *
 * @return				The global reference to the original java task 
 *						instance.
 */
jobject jtask_new_global_ref(jobject jtask, JNIEnv * env);

/**
 * This function delete a global reference to a java task instance.
 *
 * @param				The global refernce to delete.
 * @param env			The environment of the current thread.
 */
void jtask_delete_global_ref(jobject jtask, JNIEnv * env);

/**
 * This function associated a native task to a java task instance.
 *
 * @param jtask			The java task instance.
 * @param task			The native task to bind.
 * @param env			The environment of the current thread.
 *
 * @exception			If the class Task is not found the function throws 
 *						the ClassNotFoundException. If the field bind of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 */
void jtask_bind(jobject jtask, m_task_t task, JNIEnv * env);

/**
 * This function returns a native task from a java task instance.
 *
 * @param jtask			The java task object from which get the native task.
 * @param env			The environment of the current thread.
 *
 * @return				The function returns the native task associated to the
 *						java task object.
 *
 * @exception			If the class Task is not found the function throws 
 *						the ClassNotFoundException. If the field bind of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 */
m_task_t jtask_to_native_task(jobject jtask, JNIEnv * env);

/**
 * This function tests if a java task instance is valid.
 * A java task object is valid if it is bind to a native 
 * task.
 *
 * @param jtask			The java task to test the validity.
 * @param env			The environment of the current thread.
 *
 * @return				If the java task is valid the function returns true.
 *						Otherwise the function returns false.
 */
jboolean jtask_is_valid(jobject jtask, JNIEnv * env);

#endif                          /* !MSG_JTASK_H */
