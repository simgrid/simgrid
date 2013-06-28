/* Functions related to the MSG VM API. */

/* Copyright (c) 2012. The SimGrid Team. */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */
#include "jmsg_vm.h"
#include "jmsg_host.h"
#include "jmsg_process.h"
#include "jxbt_utilities.h"
#include "msg/msg.h"
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jfieldID jvm_field_bind;

void jvm_bind(JNIEnv *env, jobject jvm, msg_vm_t vm) {
  (*env)->SetLongField(env, jvm, jvm_field_bind, (intptr_t)vm);
}
msg_vm_t jvm_get_native(JNIEnv *env, jobject jvm) {
  return (msg_vm_t)(intptr_t)(*env)->GetLongField(env, jvm, jvm_field_bind);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_nativeInit(JNIEnv *env, jclass cls) {
  jclass jprocess_class_VM = (*env)->FindClass(env, "org/simgrid/msg/VM");
  jvm_field_bind = jxbt_get_jfield(env, jprocess_class_VM, "bind", "J");
  if (!jvm_field_bind	) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isCreated(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_created(vm);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isRunning(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_running(vm);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isMigrating(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_migrating(vm);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isSuspended(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_suspended(vm);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isSaving(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_saving(vm);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isSaved(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_saved(vm);
}

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_VM_isRestoring(JNIEnv * env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_restoring(vm);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_create(JNIEnv *env, jobject jvm, jobject jhost, jstring jname,
		               jint jncore, jint jramsize, jint jnetcap, jstring jdiskpath, jint jdisksize) {
  msg_host_t host = jhost_get_native(env, jhost);

  const char *name;
  name = (*env)->GetStringUTFChars(env, jname, 0);
  name = xbt_strdup(name);

  // TODO disk concerns are not taken into account yet
  // const char *diskpath;
  // disk_path = (*env)->GetStringUTFChars(env, jdiskpath, 0);
  // disk_path = xbt_strdup(disk_path);

  msg_vm_t vm = MSG_vm_create(host, name, (int) jncore, (int) jramsize,
		  (int) jnetcap, NULL, (int) jdisksize);

  jvm_bind(env,jvm,vm);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_destroy(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_destroy(vm);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_start(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_start(vm);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_shutdown(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_shutdown(vm);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_internalmig(JNIEnv *env, jobject jvm, jobject jhost) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_host_t host = jhost_get_native(env, jhost);

  XBT_INFO("Start migration of %s to %s", MSG_host_get_name(vm), MSG_host_get_name(host));
  MSG_vm_migrate(vm,host);
  XBT_INFO("Migration done");
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

JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_save(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_save(vm);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_restore(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_restore(vm);
}



JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_VM_get_pm(JNIEnv *env, jobject jvm) {
  jobject jhost;
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_host_t host = MSG_vm_get_pm(vm);

  if (!MSG_host_get_data(host)) {
    /* the native host not yet associated with the java host instance */

    /* instanciate a new java host instance */
    jhost = jhost_new_instance(env);

    if (!jhost) {
      jxbt_throw_jni(env, "java host instantiation failed");
      return NULL;
    }

    /* get a global reference to the newly created host */
    jhost = jhost_ref(env, jhost);

    if (!jhost) {
      jxbt_throw_jni(env, "global ref allocation failed");
      return NULL;
    }
    /* Sets the host name */
    const char *name = MSG_host_get_name(host);
    jobject jname = (*env)->NewStringUTF(env,name);
    (*env)->SetObjectField(env, jhost, jxbt_get_jfield(env, (*env)->FindClass(env, "org/simgrid/msg/Host"), "name", "Ljava/lang/String;"), jname);
    /* Bind & store it */
    jhost_bind(jhost, host, env);
    MSG_host_set_data(host, (void *) jhost);
  } else {
    jhost = (jobject) MSG_host_get_data(host);
  }

  return jhost;
}
