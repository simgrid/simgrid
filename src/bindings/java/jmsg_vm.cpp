/* Functions related to the Virtual Machines.                               */

/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg_vm.h"
#include "jmsg_host.h"
#include "jxbt_utilities.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/plugins/live_migration.h"
#include "src/kernel/context/Context.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

extern int JAVA_HOST_LEVEL;
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

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_create(JNIEnv* env, jobject jVm, jobject jHost, jstring jname,
                                                      jint coreAmount, jint jramsize, jint jmig_netspeed,
                                                      jint jdp_intensity)
{
  msg_host_t host = jhost_get_native(env, jHost);

  const char* name = env->GetStringUTFChars(jname, 0);
  msg_vm_t vm      = MSG_vm_create_migratable(host, name, static_cast<int>(coreAmount), static_cast<int>(jramsize),
                                         static_cast<int>(jmig_netspeed), static_cast<int>(jdp_intensity));
  env->ReleaseStringUTFChars(jname, name);

  jvm_bind(env, jVm, vm);
  jVm = env->NewGlobalRef(jVm);
  // We use the extension level of the host, even if that's somehow disturbing
  vm->extension_set(JAVA_HOST_LEVEL, (void*)jVm);
}

JNIEXPORT jobjectArray JNICALL Java_org_simgrid_msg_VM_all(JNIEnv* env, jclass cls_arg)
{
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  std::vector<jobject> vms;

  unsigned int it;
  msg_host_t h;
  xbt_dynar_foreach (hosts, it, h) {
    simgrid::s4u::VirtualMachine* vm = dynamic_cast<simgrid::s4u::VirtualMachine*>(h);
    if (vm != nullptr && vm->get_state() != simgrid::s4u::VirtualMachine::state::DESTROYED) {
      jobject jvm = static_cast<jobject>(vm->extension(JAVA_HOST_LEVEL));
      vms.push_back(jvm);
    }
  }
  xbt_dynar_free(&hosts);

  vms.shrink_to_fit();
  int count = vms.size();

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/VM");
  if (not cls)
    return nullptr;

  jobjectArray jtable = env->NewObjectArray((jsize)count, cls, nullptr);
  if (not jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return nullptr;
  }

  for (int index = 0; index < count; index++) {
    jobject jhost = vms.at(index);
    env->SetObjectArrayElement(jtable, index, jhost);
  }
  return jtable;
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
  if (vm)
    MSG_vm_shutdown(vm);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_destroy(JNIEnv* env, jobject jvm)
{
  msg_vm_t vm = jvm_get_native(env, jvm);
  if (vm) {
    MSG_vm_destroy(vm);
    auto* vmList = &simgrid::vm::VirtualMachineImpl::allVms_;
    vmList->erase(std::remove(vmList->begin(), vmList->end(), vm), vmList->end());
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_VM_nativeMigration(JNIEnv* env, jobject jvm, jobject jhost)
{
  msg_vm_t vm = jvm_get_native(env,jvm);
  msg_host_t host = jhost_get_native(env, jhost);
  if (not simgrid::ForcefulKillException::try_n_catch([&vm, &host]() { MSG_vm_migrate(vm, host); })) {
    XBT_VERB("Caught exception during migration");
    jxbt_throw_host_failure(env, "during migration");
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

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_VM_getVMByName(JNIEnv* env, jclass cls, jstring jname)
{
  /* get the C string from the java string */
  if (jname == nullptr) {
    jxbt_throw_null(env, "No VM can have a null name");
    return nullptr;
  }
  const char* name = env->GetStringUTFChars(jname, 0);
  /* get the VM by name   (VMs are just special hosts, unfortunately) */
  msg_host_t host = MSG_host_by_name(name);

  if (not host) { /* invalid name */
    jxbt_throw_host_not_found(env, name);
    env->ReleaseStringUTFChars(jname, name);
    return nullptr;
  }
  env->ReleaseStringUTFChars(jname, name);

  return static_cast<jobject>(host->extension(JAVA_HOST_LEVEL));
}
