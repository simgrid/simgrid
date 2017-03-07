/* Functions related to the Virtual Machines.                               */

/* Copyright (c) 2012-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg_vm.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "xbt/ex.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

SG_BEGIN_DECL()

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
  xbt_assert(jvm_field_bind, "Native initialization of msg/VM failed. Please report that bug");
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_VM_isCreated(JNIEnv * env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  return MSG_vm_is_created(vm);
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

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_setBound(JNIEnv *env, jobject jvm, jdouble bound)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  MSG_vm_set_bound(vm, bound);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_create(JNIEnv* env, jobject jvm, jobject jhost, jstring jname,
                                                      jint jramsize, jint jmig_netspeed, jint jdp_intensity)
{
  msg_host_t host = jhost_get_native(env, jhost);

  const char* name = env->GetStringUTFChars(jname, 0);
  msg_vm_t vm = MSG_vm_create(host, name, static_cast<int>(jramsize), static_cast<int>(jmig_netspeed),
                              static_cast<int>(jdp_intensity));
  env->ReleaseStringUTFChars(jname, name);

  jvm_bind(env, jvm, vm);
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
  if (vm) {
    MSG_vm_shutdown(vm);
    auto vmList = &simgrid::vm::VirtualMachineImpl::allVms_;
    vmList->erase(
        std::remove_if(vmList->begin(), vmList->end(), [vm](simgrid::s4u::VirtualMachine* it) { return vm == it; }),
        vmList->end());
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_internalmig(JNIEnv *env, jobject jvm, jobject jhost)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_host_t host = jhost_get_native(env, jhost);
  try {
    MSG_vm_migrate(vm,host);
  }
  catch(xbt_ex& e){
     XBT_VERB("CATCH EXCEPTION MIGRATION %s",e.what());
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

SG_END_DECL()
