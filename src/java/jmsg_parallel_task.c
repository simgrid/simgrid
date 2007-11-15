/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the implementation of the functions in relation with the java
 * process instance. 
 */

#include "xbt/log.h" 
#include "jmsg.h"
#include "jmsg_parallel_task.h"
#include "jxbt_utilities.h"

jobject
jparallel_task_ref(JNIEnv* env, jobject jparallel_task) {
  jobject newref = (*env)->NewGlobalRef(env,jparallel_task); 

  DEBUG1("jparallel_task_ref(%ld)",(long)newref);

  return newref;
}

void
jparallel_task_unref(JNIEnv* env, jobject jparallel_task) {
  DEBUG1("jparallel_task_unref(%ld)",(long)jparallel_task);
  (*env)->DeleteGlobalRef(env,jparallel_task);
}

void
jparallel_task_bind(jobject jparallel_task,m_task_t task,JNIEnv* env) {
  jfieldID id  = jxbt_get_sfield(env,"simgrid/msg/ParallelTask","bind", "J");
	
  if(!id)
    return;
	
  (*env)->SetLongField(env,jparallel_task,id,(jlong)(long)(task));
	 
}

m_task_t
jparallel_task_to_native_parallel_task(jobject jparallel_task,JNIEnv* env) {
  jfieldID id  = jxbt_get_sfield(env,"simgrid/msg/ParallelTask","bind", "J");
	
  if(!id)
    return NULL;
    
  return (m_task_t)(long)(*env)->GetLongField(env,jparallel_task,id);
}

jboolean
jparallel_task_is_valid(jobject jparallel_task,JNIEnv* env) {
  jfieldID id  = jxbt_get_sfield(env,"simgrid/msg/ParallelTask","bind", "J");
	
  if(!id)
    return JNI_FALSE;

  return (*env)->GetLongField(env,jparallel_task,id) ? JNI_TRUE : JNI_FALSE;
}
