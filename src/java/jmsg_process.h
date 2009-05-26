/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the declarations of the functions in relation with the java
 * process instance.
 */

#ifndef MSG_JPROCESS_H
#define MSG_JPROCESS_H

#include <jni.h>
#include "msg/msg.h"
#include "xbt/context.h"

/**
 * This function returns a global reference to the  java process instance 
 * specified by the parameter jprocess.
 *
 * @param jprocess		The original java process instance.
 * @param env			The env of the current thread
 *
 * @return				The global reference to the original java process 
 *						instance.
 */
jobject jprocess_new_global_ref(jobject jprocess, JNIEnv * env);

/**
 * This function delete a global reference to a java process instance.
 * If the java process is alive the function joins it and stops it before.
 *
 * @param				The global refernce to delete.
 * @param env			The env of the current thread
 *
 * @see					jprocess_join()
 * @see					jprocess_exit()
 */
void jprocess_delete_global_ref(jobject jprocess, JNIEnv * env);

/**
 *
 * This function tests if the specified java process instance is alive. 
 * A java process object is alive if it has been started and has not yet 
 * terminated.
 * 
 * @param jprocess		The java process to test.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the methos isAlive() of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.	
 *
 * @return				If the java process is alive the function returns
 *						true. Otherwise the function returns false.
 */
jboolean jprocess_is_alive(jobject jprocess, JNIEnv * env);

/**
 * This function waits for a java process to terminate.
 *
 * @param jprocess		The java process ot wait for.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the methos join() of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.	
 *
 */
void jprocess_join(jobject jprocess, JNIEnv * env);

/**
 * This function starts the specified java process.
 *
 * @param jprocess		The java process to start.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the methos start() of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.	
 */
void jprocess_start(jobject jprocess, JNIEnv * env);

/**
 * This function forces the java process to stop.
 *
 * @param jprocess		The java process to stop.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the methos stop() of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.	
 */
void jprocess_exit(jobject jprocess, JNIEnv * env);

/**
 * This function associated a native process to a java process instance.
 *
 * @param jprocess		The java process instance.
 * @param process		The native process to bind.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the field bind of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 */
void jprocess_bind(jobject jprocess, m_process_t process, JNIEnv * env);

/**
 * This function returns a native process from a java process instance.
 *
 * @param jprocess		The java process object from which get the native process.
 * @param env			The env of the current thread
 *
 * @return				The function returns the native process associated to the
 *						java process object.
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the field bind of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 */
m_process_t jprocess_to_native_process(jobject jprocess, JNIEnv * env);

/**
 * This function gets the id of the specified java process.
 *
 * @param jprocess		The java process to get the id.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the field id of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 *
 * @return				The id of the specified java process.
 */
jlong jprocess_get_id(jobject jprocess, JNIEnv * env);

/**
 * This function tests if a java process instance is valid.
 * A java process object is valid if it is bind to a native 
 * process.
 *
 * @param jprocess		The java process to test the validity.
 * @param env			The env of the current thread
 *
 * @return				If the java process is valid the function returns true.
 *						Otherwise the function returns false.
 */
jboolean jprocess_is_valid(jobject jprocess, JNIEnv * env);

/**
 * This function gets the name of the specified java process.
 *
 * @param jprocess		The java process to get the name.
 * @param env			The env of the current thread
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the field name of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 *
 * @return				The name of the specified java process.
 */
jstring jprocess_get_name(jobject jprocess, JNIEnv * env);

/**
 * This function yields the specified java process.
 *
 * @param jprocess		The java process to yield.
 * @param env			The env of the current thread.
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the method switchProcess of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.
 */
void jprocess_yield(jobject jprocess, JNIEnv * env);

/**
 * This function locks the mutex of the specified java process.
 *
 * @param jprocess		The java process of the mutex to lock.
 * @param env			The env of the current thread.
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the method lockMutex of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.
 */
void jprocess_lock_mutex(jobject jprocess, JNIEnv * env);

/**
 * This function unlocks the mutex of the specified java process.
 *
 * @param jprocess		The java process of the mutex to unlock.
 * @param env			The env of the current thread.
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the method unlockMutex of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.
 */
void jprocess_unlock_mutex(jobject jprocess, JNIEnv * env);

/**
 * This function signals the condition of the mutex of the specified java process.
 *
 * @param jprocess		The java process of the condtion to signal.
 * @param env			The env of the current thread.
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the method signalCond of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.
 */
void jprocess_signal_cond(jobject jprocess, JNIEnv * env);

/**
 * This function waits the condition of the mutex of the specified java process.
 *
 * @param jprocess		The java process of the condtion to wait for.
 * @param env			The env of the current thread.
 *
 * @exception			If the class Process is not found the function throws 
 *						the ClassNotFoundException. If the method waitCond of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.
 */
void jprocess_wait_cond(jobject jprocess, JNIEnv * env);

void jprocess_schedule(xbt_context_t context);

void jprocess_unschedule(xbt_context_t context);


#endif /* !MSG_JPROCESS_H */
