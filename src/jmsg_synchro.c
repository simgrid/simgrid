/* Functions exporting the simgrid synchronization mechanisms to the Java world */

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.                   */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package.    */

#include "jmsg_synchro.h"
#include "jxbt_utilities.h"


JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_init(JNIEnv * env, jobject obj) {
	xbt_mutex_t mutex = xbt_mutex_init();

	jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Mutex", "bind", "J");
	if (!id) return;

	(*env)->SetLongField(env, obj, id, (jlong) (long) (mutex));
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_acquire(JNIEnv * env, jobject obj) {
	xbt_mutex_t mutex;

	jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Mutex", "bind", "J");
	if (!id) return;

	mutex = (xbt_mutex_t) (long) (*env)->GetLongField(env, obj, id);
	xbt_mutex_acquire(mutex);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_release(JNIEnv * env, jobject obj) {
	xbt_mutex_t mutex;

	jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Mutex", "bind", "J");
	if (!id) return;

	mutex = (xbt_mutex_t) (long) (*env)->GetLongField(env, obj, id);
	xbt_mutex_release(mutex);
}
