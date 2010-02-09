/*
 * Copyright 2006,2007,2010 Da SimGrid Team.
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Upcalls to the Java functions used as callback to the FleXML application file parser.
 *
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
