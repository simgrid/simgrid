/* Functions related to the MSG VM API. */

/* Copyright (c) 2012. The SimGrid Team. */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */
#include "jmsg_vm.h"
#include "jmsg_host.h"
#include "jmsg_process.h"
#include "jxbt_utilities.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

void jvm_bind(JNIEnv *env, jobject jvm, msg_vm_t vm) {
  (*env)->SetLongField(env, jvm, jvm_field_bind, (jlong) (long) (vm));
}
msg_vm_t jvm_get_native(JNIEnv *env, jobject jvm) {
	msg_vm_t vm = (msg_vm_t)(*env)->GetLongField(env, jvm, jvm_field_bind);
	return vm;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_nativeInit(JNIEnv *env, jclass cls) {
	jclass jprocess_class_VM = (*env)->FindClass(env, "org/simgrid/msg/VM");
	jvm_field_bind = jxbt_get_jfield(env, jprocess_class_VM, "bind", "J");
	if (!jvm_field_bind	) {
  	jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
	}
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_start(JNIEnv *env, jobject jvm, jobject jhost, jint jcoreamount) {
	m_host_t host = jhost_get_native(env, jhost);

	msg_vm_t vm = MSG_vm_start(host, (int)jcoreamount);

	jvm_bind(env,jvm,vm);
}
JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_VM_isSuspended(JNIEnv *env, jobject jvm) {
	msg_vm_t vm = jvm_get_native(env,jvm);

	return MSG_vm_is_suspended(vm) ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_VM_isRunning(JNIEnv *env, jobject jvm) {
	msg_vm_t vm = jvm_get_native(env,jvm);

	return MSG_vm_is_running(vm) ? JNI_TRUE : JNI_FALSE;
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_bind(JNIEnv *env, jobject jvm, jobject jprocess) {
	msg_vm_t vm = jvm_get_native(env,jvm);
	m_process_t process = jprocess_to_native_process(jprocess,env);

	MSG_vm_bind(vm,process);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_unbind(JNIEnv *env, jobject jvm, jobject jprocess) {
	msg_vm_t vm = jvm_get_native(env,jvm);
	m_process_t process = jprocess_to_native_process(jprocess,env);

	MSG_vm_unbind(vm,process);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_migrate(JNIEnv *env, jobject jvm, jobject jhost) {
	msg_vm_t vm = jvm_get_native(env,jvm);
	m_host_t host = jhost_get_native(env, jhost);

	MSG_vm_migrate(vm,host);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_suspend(JNIEnv *env, jobject jvm) {
	msg_vm_t vm = jvm_get_native(env,jvm);

	MSG_vm_suspend(vm);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_resume(JNIEnv *env, jobject jvm) {
	msg_vm_t vm = jvm_get_native(env,jvm);

	MSG_vm_resume(vm);
}
