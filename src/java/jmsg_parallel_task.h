/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the declarations of the functions in relation with the java
 * parallel task instance.
 */

#ifndef MSG_JPARALLEL_TASK_H
#define MSG_JPARALLEL_TASK_H

#include <jni.h>
#include "msg/msg.h"


/**
 * This function returns a global reference to the java paralllel task instance 
 * specified by the parameter jparallel_task.
 *
 * @param jparallel_task	The original java parallel task instance.
 * @param env				The environment of the current thread.
 *
 * @return					The global reference to the original java parallel task 
 *							instance.
 */			
jobject
jparallel_task_ref(JNIEnv* env, jobject jparallel_task);

/**
 * This function delete a global reference to a java parallel task instance.
 *
 * @param jparallel_task	The global refernce to delete.
 * @param env				The environment of the current thread.
 */
void
jparallel_task_unref(JNIEnv* env,jobject jparallel_task);

/**
 * This function associated a native task to a java parallel task instance.
 *
 * @param jparallel_task	The java parallel task instance.
 * @param jparallel_task	The native parallel task to bind.
 * @param env				The environment of the current thread.
 *
 * @exception				If the class ParallelTask is not found the function throws 
 *							the ClassNotFoundException. If the field bind of 
 *							this class is not found the function throws the exception 
 *							NotSuchFieldException.	
 */		
void
jparallel_task_bind(jobject jparallel_task,m_task_t task,JNIEnv* env);

/**
 * This function returns a native task from a java task instance.
 *
 * @param jparallel_task	The java parallel task object from which get the native parallel task.
 * @param env				The environment of the current thread.
 *
 * @return					The function returns the native parallel task associated to the
 *							java parallel task object.
 *
 * @exception				If the class ParallelTask is not found the function throws 
 *							the ClassNotFoundException. If the field bind of 
 *							this class is not found the function throws the exception 
 *							NotSuchFieldException.	
 */
m_task_t
jparallel_task_to_native_parallel_task(jobject jparallel_task,JNIEnv* env);

/**
 * This function tests if a java task instance is valid.
 * A java task object is valid if it is bind to a native 
 * task.
 *
 * @param jparallel_task	The java parallel task to test the validity.
 * @param env				The environment of the current thread.
 *
 * @return					If the java parallel task is valid the function returns true.
 *							Otherwise the function returns false.
 */
jboolean
jparallel_task_is_valid(jobject jparallel_task,JNIEnv* env);


#endif /* !MSG_JPARALLEL_TASK_H */
