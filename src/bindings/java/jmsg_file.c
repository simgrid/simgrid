/* Functions related to the java file API.                            */
/* Copyright (c) 2012-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */
#include "jmsg_file.h"
#include "jxbt_utilities.h"

void jfile_bind(JNIEnv *env, jobject jfile, msg_file_t fd) {
  (*env)->SetLongField(env, jfile, jfile_field_bind, (intptr_t)fd);
}

msg_file_t jfile_get_native(JNIEnv *env, jobject jfile) {
  msg_file_t file =
    (msg_file_t)(intptr_t)(*env)->GetLongField(env, jfile, jfile_field_bind);
  return file;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_File_nativeInit(JNIEnv *env, jclass cls) {
  jclass class_File = (*env)->FindClass(env, "org/simgrid/msg/File");
  jfile_field_bind = jxbt_get_jfield(env , class_File, "bind", "J");
  xbt_assert((jfile_field_bind != NULL), "Can't find \"bind\" field in File class.");
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_File_open(JNIEnv *env, jobject jfile, jobject jstorage, jobject jpath) {
  const char *storage = (*env)->GetStringUTFChars(env, jstorage, 0);
  const char *path = (*env)->GetStringUTFChars(env, jpath, 0);
  msg_file_t file;

  file = MSG_file_open(storage, path, NULL);
  jfile_bind(env, jfile, file);

  (*env)->ReleaseStringUTFChars(env, jstorage, storage);
  (*env)->ReleaseStringUTFChars(env, jpath, path);
}
JNIEXPORT jlong JNICALL
Java_org_simgrid_msg_File_read(JNIEnv *env, jobject jfile, jlong jsize) {
  msg_file_t file = jfile_get_native(env, jfile);
  return (jlong)MSG_file_read(file, (sg_storage_size_t)jsize);
}

JNIEXPORT jlong JNICALL
Java_org_simgrid_msg_File_write(JNIEnv *env, jobject jfile, jlong jsize) {
  msg_file_t file = jfile_get_native(env, jfile);
  return (jlong)MSG_file_write(file, (sg_storage_size_t)jsize);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_File_close(JNIEnv *env, jobject jfile) {
  msg_file_t file = jfile_get_native(env, jfile);

  MSG_file_close(file);
  jfile_bind(env, jfile, NULL);
}

