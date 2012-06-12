/* Functions exporting the simgrid synchronization mechanisms to the Java world */

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.                   */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package.    */

#include "jmsg_synchro.h"
#include "jxbt_utilities.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jfieldID jsyncro_field_Mutex_bind;

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_nativeInit(JNIEnv *env, jclass cls) {
	jsyncro_field_Mutex_bind = jxbt_get_sfield(env, "org/simgrid/msg/Mutex", "bind", "J");
	if (!jsyncro_field_Mutex_bind) {
  	jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
	}
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_init(JNIEnv * env, jobject obj) {
	xbt_mutex_t mutex = xbt_mutex_init();

	(*env)->SetLongField(env, obj, jsyncro_field_Mutex_bind, (jlong) (long) (mutex));
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_acquire(JNIEnv * env, jobject obj) {
	xbt_mutex_t mutex;

	mutex = (xbt_mutex_t) (long) (*env)->GetLongField(env, obj, jsyncro_field_Mutex_bind);
	xbt_mutex_acquire(mutex);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_release(JNIEnv * env, jobject obj) {
	xbt_mutex_t mutex;

	mutex = (xbt_mutex_t) (long) (*env)->GetLongField(env, obj, jsyncro_field_Mutex_bind);
	xbt_mutex_release(mutex);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Mutex_exit(JNIEnv * env, jobject obj) {
		xbt_mutex_t mutex;

		mutex = (xbt_mutex_t) (long) (*env)->GetLongField(env, obj, jsyncro_field_Mutex_bind);
		xbt_mutex_destroy(mutex);
}
