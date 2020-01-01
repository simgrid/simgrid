/* Java bindings of the file API.                                           */

/* Copyright (c) 2012-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg_file.h"
#include "jxbt_utilities.hpp"

void jfile_bind(JNIEnv *env, jobject jfile, msg_file_t fd) {
  env->SetLongField(jfile, jfile_field_bind, reinterpret_cast<std::intptr_t>(fd));
}

msg_file_t jfile_get_native(JNIEnv *env, jobject jfile) {
  return reinterpret_cast<msg_file_t>(env->GetLongField(jfile, jfile_field_bind));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_File_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_File = env->FindClass("org/simgrid/msg/File");
  jfile_field_bind = jxbt_get_jfield(env , class_File, "bind", "J");
  xbt_assert((jfile_field_bind != nullptr), "Can't find 'bind' field in File class.");
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_File_open(JNIEnv *env, jobject jfile, jobject jpath) {
  const char *path = env->GetStringUTFChars((jstring) jpath, 0);
  msg_file_t file  = MSG_file_open(path, nullptr);
  jfile_bind(env, jfile, file);

  env->ReleaseStringUTFChars(static_cast<jstring>(jpath), path);
}

JNIEXPORT jlong JNICALL Java_org_simgrid_msg_File_read(JNIEnv *env, jobject jfile, jlong jsize) {
  msg_file_t file = jfile_get_native(env, jfile);
  return static_cast<jlong>(MSG_file_read(file, static_cast<sg_size_t>(jsize)));
}

JNIEXPORT jlong JNICALL Java_org_simgrid_msg_File_write(JNIEnv *env, jobject jfile, jlong jsize) {
  msg_file_t file = jfile_get_native(env, jfile);
  return static_cast<jlong>(MSG_file_write(file, static_cast<sg_size_t>(jsize)));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_File_seek(JNIEnv *env, jobject jfile, jlong offset, jlong origin) {
  msg_file_t file = jfile_get_native(env, jfile);
  MSG_file_seek(file, static_cast<sg_offset_t>(offset), static_cast<int>(origin));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_File_close(JNIEnv *env, jobject jfile) {
  const_sg_file_t file = jfile_get_native(env, jfile);

  MSG_file_close(file);
  jfile_bind(env, jfile, nullptr);
}
