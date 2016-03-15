/* Functions related to the java file API.                            */

/* Copyright (c) 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MSG_JFILE_H
#define MSG_JFILE_H
#include <jni.h>
#include "simgrid/msg.h"

SG_BEGIN_DECL()

jfieldID jfile_field_bind;

void jfile_bind(JNIEnv *env, jobject jfile, msg_file_t fd);
msg_file_t jfile_get_native(JNIEnv *env, jobject jfile);
/**
 * Class      org_simgrid_msg_File
 * Method      nativeInit
 * Signature  ()V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_File_nativeInit(JNIEnv*, jclass);

/**
 * Class      org_simgrid_msg_File
 * Method      open
 * Signature  (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_File_open(JNIEnv*, jobject, jobject);

/**
 * Class       org_simgrid_msg_File
 * Method      read
 */
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_File_read(JNIEnv*, jobject, jlong);

/**
 * Class      org_simgrid_msg_File
 * Method      write
 */
JNIEXPORT jlong JNICALL Java_org_simgrid_msg_File_write(JNIEnv*, jobject, jlong);

/**
 * Class      org_simgrid_msg_File
 * Method      close
 */
JNIEXPORT void JNICALL Java_org_simgrid_msg_File_close(JNIEnv*, jobject);

SG_END_DECL()
#endif
