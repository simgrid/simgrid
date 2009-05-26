/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the declarations of the functions in relation with the java
 * host instance.
 */

#ifndef MSG_JHOST_H
#define MSG_JHOST_H

#include <jni.h>
#include "msg/msg.h"

/**
 * This function returns a new java host instance.
 *
 * @param env			The environment of the current thread
 *
 * @return				A new java host object.
 *
 * @exception			If the class Host is not found the function throws 
 *						the ClassNotFoundException. If the constructor of 
 *						this class is not found the function throws the exception 
 *						NotSuchMethodException.
 */
jobject jhost_new_instance(JNIEnv * env);

/**
 * This function returns a global reference to the  java host instance 
 * specified by the parameter jhost.
 *
 * @param jhost			The original java host instance.
 * @param env			The environment of the current thread
 *
 * @return				The global reference to the original java host 
 *						instance.
 */
jobject jhost_ref(JNIEnv * env, jobject jhost);
/**
 * This function delete a global reference to a java host instance.
 *
 * @param				The global refernce to delete.
 * @param env			The environment of the current thread
 */
void jhost_unref(JNIEnv * env, jobject jhost);

/**
 * This function associated a native host to a java host instance.
 *
 * @param jhost			The java host instance.
 * @param host			The native host to bind.
 * @param env			The environment of the current thread
 *
 * @exception			If the class Host is not found the function throws 
 *						the ClassNotFoundException. If the field bind of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 */
void jhost_bind(jobject jhost, m_host_t host, JNIEnv * env);

/**
 * This function returns a native host from a java host instance.
 *
 * @param jhost			The java host object from which get the native host.
 * @param env			The environment of the current thread
 *
 * @return				The function returns the native host associated to the
 *						java host object.
 *
 * @exception			If the class Host is not found the function throws 
 *						the ClassNotFoundException. If the field bind of 
 *						this class is not found the function throws the exception 
 *						NotSuchFieldException.	
 */
m_host_t jhost_get_native(JNIEnv * env, jobject jhost);

/**
 * This function returns the name of a MSG host.
 *
 * @param jhost			A java host object.
 * @param env			The environment of the current thread
 *
 * @return				The name of the host.
 */
const char *jhost_get_name(jobject jhost, JNIEnv * env);


/**
 * This function sets the name of a MSG	host.
 *
 * @param host			The host concerned by the operation.
 * @param jname			The new name of the host.
 * @param env			The environment of the current thread
 */
void jhost_set_name(jobject jhost, jstring jname, JNIEnv * env);

/**
 * This function tests if a java host instance is valid.
 * A java host object is valid if it is bind to a native host.
 *
 * @param jhost			The host to test the validity.
 * @param env			The environment of the current thread
 *
 * @return				If the java host is valid the function returns true.
 *						Otherwise the function returns false.
 */
jboolean jhost_is_valid(jobject jhost, JNIEnv * env);

#endif /*!MSG_JHOST_H */
