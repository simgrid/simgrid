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
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_start(JNIEnv *env, jobject jvm, jobject jhost, jstring jname, jint jcoreamount) {
  msg_host_t host = jhost_get_native(env, jhost);

  const char *name;
  name = (*env)->GetStringUTFChars(env, jname, 0);
  name = xbt_strdup(name);
  
  msg_vm_t vm = MSG_vm_start(host, name, (int)jcoreamount);

  jvm_bind(env,jvm,vm);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_destroy(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_destroy(vm);
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
  msg_process_t process = jprocess_to_native_process(jprocess,env);

  xbt_assert((vm != NULL), "VM object is not bound");
  xbt_assert((process != NULL), "Process object is not bound.");

  MSG_vm_bind(vm,process);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_unbind(JNIEnv *env, jobject jvm, jobject jprocess) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_process_t process = jprocess_to_native_process(jprocess,env);

  MSG_vm_unbind(vm,process);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_migrate(JNIEnv *env, jobject jvm, jobject jhost) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_host_t host = jhost_get_native(env, jhost);

  MSG_vm_migrate(vm,host);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_suspend(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  xbt_ex_t e;
  TRY {
    MSG_vm_suspend(vm);
  }
  CATCH(e) {
    xbt_ex_free(e);
  }
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_resume(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  xbt_ex_t e;
  TRY {
    MSG_vm_resume(vm);
  }
  CATCH(e) {
    xbt_ex_free(e);
  }
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_shutdown(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  xbt_ex_t e;
  TRY {
    MSG_vm_shutdown(vm);
  }
  CATCH(e) {
    xbt_ex_free(e);
  }
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_VM_reboot(JNIEnv *env, jobject jvm) {
  msg_vm_t vm = jvm_get_native(env,jvm);
  xbt_ex_t e;
  TRY {
    MSG_vm_reboot(vm);
  }
  CATCH(e) {
    xbt_ex_free(e);
  }
}
