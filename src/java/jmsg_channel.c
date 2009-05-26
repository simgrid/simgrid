/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the implementation of the functions in relation with the java
 * channel instance. 
 */

#include "jmsg_channel.h"
#include "jmsg.h"
#include "jxbt_utilities.h"

jint jchannel_get_id(jobject jchannel, JNIEnv * env)
{
  jmethodID id = jxbt_get_smethod(env, "simgrid/msg/Channel", "getId", "()I");

  if (!id)
    return -1;

  return (*env)->CallIntMethod(env, jchannel, id);
}
