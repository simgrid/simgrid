/* Functions related to the MSG VM API. */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg.h"
#include "jmsg_vm.h"
#include "jmsg_host.h"
#include "jmsg_process.h"
#include "jxbt_utilities.h"
#include "simgrid/msg.h"
#include <surf/surf_routing.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jfieldID jvm_field_bind;

void jvm_bind(JNIEnv *env, jobject jvm, msg_vm_t vm)
{
  env->SetLongField(jvm, jvm_field_bind, (intptr_t)vm);
}

msg_vm_t jvm_get_native(JNIEnv *env, jobject jvm)
{
  return (msg_vm_t)(intptr_t)env->GetLongField(jvm, jvm_field_bind);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_nativeInit(JNIEnv *env, jclass cls)
{
  jclass jprocess_class_VM = env->FindClass("org/simgrid/msg/VM");
  jvm_field_bind = jxbt_get_jfield(env, jprocess_class_VM, "bind", "J");
  if (!jvm_field_bind  ) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isCreated(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_created(vm);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isRunning(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_running(vm);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isMigrating(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_migrating(vm);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isSuspended(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_suspended(vm);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isSaving(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_saving(vm);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isSaved(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_saved(vm);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isRestoring(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return (jint) MSG_vm_is_restoring(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_setBound(JNIEnv *env, jobject jvm, jdouble bound)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_set_bound(vm, bound);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_create(JNIEnv *env, jobject jvm, jobject jhost, jstring jname,
                                                      jint jncore, jint jramsize, jint jnetcap, jstring jdiskpath,
                                                      jint jdisksize, jint jmig_netspeed, jint jdp_intensity)
{
  msg_host_t host = jhost_get_native(env, jhost);

  const char *name;
  name = env->GetStringUTFChars(jname, 0);
  name = xbt_strdup(name);

  // TODO disk concerns are not taken into account yet
  // const char *diskpath;
  // disk_path = (*env)->GetStringUTFChars(env, jdiskpath, 0);
  // disk_path = xbt_strdup(disk_path);

  msg_vm_t vm = MSG_vm_create(host, name, (int) jncore, (int) jramsize, (int) jnetcap, NULL, (int) jdisksize,
                              (int) jmig_netspeed, (int) jdp_intensity);

  jvm_bind(env,jvm,vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_nativeFinalize(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_destroy(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_start(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_start(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_shutdown(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_shutdown(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_internalmig(JNIEnv *env, jobject jvm, jobject jhost)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_host_t host = jhost_get_native(env, jhost);
  xbt_ex_t e;
  TRY{
    MSG_vm_migrate(vm,host);
  } CATCH(e){
     XBT_VERB("CATCH EXCEPTION MIGRATION %s",e.msg);
     xbt_ex_free(e);
     jxbt_throw_host_failure(env, (char*)"during migration");
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_suspend(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_suspend(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_resume(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_resume(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_save(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_save(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_restore(JNIEnv *env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_restore(vm);
}
