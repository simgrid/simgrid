/*
 * $Id: jmsg_application_handler.h 3684 2007-07-08 20:51:53Z mquinson $
 *
 * Copyright 2006,2007 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * This contains the declarations of the functions in relation with the java
 * host instance.
 */  
  
#ifndef MSG_JAPPLICATION_HANDLER_H
#define MSG_JAPPLICATION_HANDLER_H
  
#include <jni.h>
#include "msg/msg.h"
void japplication_handler_on_start_document(void);
void japplication_handler_on_end_document(void);
void japplication_handler_on_begin_process(void);
void japplication_handler_on_process_arg(void);
void japplication_handler_on_property(void);
void japplication_handler_on_end_process(void);

#endif  /* !MSG_JAPPLICATION_HANDLER_H */
