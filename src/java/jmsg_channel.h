/*
 * $Id$
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the declarations of the functions in relation with the java
 * channel instance.
 */

#ifndef MSG_JCHANNEL_H
#define MSG_JCHANNEL_H

#include <jni.h>

/**
 * This function returns the id of a java channel instance.
 *
 * @param jchannel		The channel to get the id.
 * @param env			The environment of the current thread.
 *
 * @return				The id of the channel.
 */
jint jchannel_get_id(jobject jchannel, JNIEnv * env);

#endif /* !MSG_JCHANNEL_H */
